//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file SupportingApps.cpp
	SupportingApps class implementation
*/

#include "mime/SupportingApps.h"

#include <Directory.h>
#include <Message.h>
#include <mime/database_support.h>
#include <MimeType.h>
#include <Path.h>
#include <storage_support.h>

#include <new>
#include <stdio.h>

#define DBG(x) x
//#define DBG(x)
#define OUT printf

namespace BPrivate {
namespace Storage {
namespace Mime {

/*!
	\class SupportingApps
	\brief Supporting apps information for the entire database
*/

// Constructor
//! Constructs a new SupportingApps object
SupportingApps::SupportingApps()
	: fHaveDoneFullBuild(false)
{
}

// Destructor
//! Destroys the SupportingApps object
SupportingApps::~SupportingApps()
{
}

// GetSupportingApps
/*! \brief Returns a list of signatures of supporting applications for the
	given type in the pre-allocated \c BMessage pointed to by \c apps.
	
	See \c BMimeType::GetSupportingApps(BMessage*) for more information.
*/
status_t 
SupportingApps::GetSupportingApps(const char *type, BMessage *apps)
{
	status_t err = type && apps ? B_OK : B_BAD_VALUE;
	// See if we need to do our initial build still
	if (!err && !fHaveDoneFullBuild) {
		err = BuildSupportingAppsTable();
	}
	if (!err) {
		// Clear the message, as we're just going to add to it
		apps->MakeEmpty();
		
		BMimeType mime(type);
		err = mime.InitCheck();
		if (!err) {
			if (mime.IsSupertypeOnly()) {
				// Add the apps that support this supertype (plus their count)
				std::set<std::string> &superApps = fSupportingApps[type];
				int32 count = 0;
				std::set<std::string>::const_iterator i;
				for (i = superApps.begin(); i != superApps.end() && !err; i++) {
					err = apps->AddString(kApplicationsField, (*i).c_str());
					count++;
				}
				if (!err)
					err = apps->AddInt32(kSupportingAppsSuperCountField, count);		
			} else {
				// Add the apps that support this subtype (plus their count)
				std::set<std::string> &subApps = fSupportingApps[type];
				int32 count = 0;
				std::set<std::string>::const_iterator i;
				for (i = subApps.begin(); i != subApps.end() && !err; i++) {
					err = apps->AddString(kApplicationsField, (*i).c_str());
					count++;
				}
				if (!err)
					err = apps->AddInt32(kSupportingAppsSubCountField, count);
				
				// Now add any apps that support the supertype, but not the
				// subtype (plus their count).
				BMimeType superMime;
				err = mime.GetSupertype(&superMime);
				if (!err)
					err = superMime.InitCheck();
				if (!err) {
					std::set<std::string> &superApps = fSupportingApps[type];
					count = 0;
					for (i = superApps.begin(); i != superApps.end() && !err; i++) {
						if (subApps.find(*i) == subApps.end()) {
							err = apps->AddString(kApplicationsField, (*i).c_str());
							count++;
						}
					}
					if (!err)
						err = apps->AddInt32(kSupportingAppsSuperCountField, count);
				}			
			}
		}
	}
	return err;
}

// SetSupportedTypes
/*! \brief Sets the list of supported types for the given application.
	\param app The application whose supported types you are setting
	\param types Pointer to a \c BMessage containing an array of supported
	             mime types in its \c Mime::kTypesField field.
	\param fullSync If \c true, \c app is removed as a supporting application
	                of any types for which it is no longer a supporting application
	                as of this call to SetSupportedTypes(). If \c false, said
	                mappings are not updated.
*/
status_t
SupportingApps::SetSupportedTypes(const char *app, const BMessage *types, bool fullSync)
{
	status_t err = app && types ? B_OK : B_BAD_VALUE;
	if (!fHaveDoneFullBuild)
		return err;
		
	std::set<std::string> oldTypes;
	std::set<std::string> &newTypes = fSupportedTypes[app];
	// Make a copy of the previous types if we're doing a full sync
	if (!err && fullSync)
		oldTypes = newTypes;
	if (!err) {
		// Read through the list of new supported types, creating the new
		// supported types list and adding the app as a supporting app for
		// each type.
		newTypes.clear();
		const char *type;
		for (int32 i = 0;
			 types->FindString(kTypesField, i, &type) == B_OK;
			 i++) {
			newTypes.insert(type);
			AddSupportingApp(type, app);
		}
		
		// Now, if we're doing a full sync, remove the app as a supporting
		// app for any types it no longer supports
		if (fullSync) {
			for (std::set<std::string>::iterator i = oldTypes.begin();
				   i != oldTypes.end();
				     i++)
			{
				if (newTypes.find(*i) == newTypes.end())			
					RemoveSupportingApp((*i).c_str(), app);
			}		
		}	
	}	
	return err;
}

// AddSupportingApp
/*! \brief Adds the given application signature to the set of supporting
	apps for the given type.
	
	\param type The full mime type
	\param app The full application signature (i.e. "application/app-subtype")	
	\return
	- B_OK: success, even if the app was already in the supporting apps list
	- "error code": failure
*/
status_t
SupportingApps::AddSupportingApp(const char *type, const char *app)
{
	status_t err = type && app ? B_OK : B_BAD_VALUE;
	if (!err) 
		fSupportingApps[type].insert(app);
	return err;
}

// RemoveSupportingApp
/*! \brief Removes the given application signature from the set of supporting
	apps for the given type.
	
	\param type The full mime type
	\param app The full application signature (i.e. "application/app-subtype")
	\return
	- B_OK: success, even if the app was not found in the supporting apps list
	- "error code": failure
*/
status_t
SupportingApps::RemoveSupportingApp(const char *type, const char *app)
{
	status_t err = app ? B_OK : B_BAD_VALUE;
	if (!err) 
		fSupportingApps[type].erase(app);
	return err;
}

// BuildSupportingAppsTable
/*! \brief Crawls the mime database and builds a list of supporting application
	signatures for every supported type.
*/
status_t
SupportingApps::BuildSupportingAppsTable()
{
	fSupportedTypes.clear();
	fSupportingApps.clear();
	
	BDirectory dir;
	status_t err = dir.SetTo(kApplicationDatabaseDir.c_str());
	
	// Build the supporting apps table based on the mime database
	if (!err) {
		dir.Rewind();
		
		// Handle each application type
		while (true) {		
			entry_ref ref;
			err = dir.GetNextRef(&ref);
			if (err) {
				// If we've come to the end of list, it's not an error
				if (err == B_ENTRY_NOT_FOUND) 
					err = B_OK;
				break;
			} else {
				BPath path;
				BMessage msg;
				char appSig[B_PATH_NAME_LENGTH+1];
				err = path.SetTo(&ref);
				if (!err) {
					// Construct a mime type string
					const char *appName = path.Leaf();
					sprintf(appSig, "application/%s", appName);

					// Read in the list of supported types
					if (read_mime_attr_message(appSig, kSupportedTypesAttr, &msg) == B_OK) {
						// Iterate through the supported types, adding them to the list of
						// supported types for the application and adding the application's
						// signature to the list of supporting apps for each type
						const char *type;
						std::set<std::string> &supportedTypes = fSupportedTypes[appName];
						for (int i = 0; msg.FindString(kTypesField, i, &type) == B_OK; i++) {
							supportedTypes.insert(type);
							AddSupportingApp(type, appSig);						
						}					
					}
				}				
			}
		}		
	}
	if (err)
		DBG(OUT("Mime::SupportingApps::BuildSupportingAppsTable() failed, error code == 0x%lx\n", err));
	else
		fHaveDoneFullBuild = true;
	return err;
}

} // namespace Mime
} // namespace Storage
} // namespace BPrivate

