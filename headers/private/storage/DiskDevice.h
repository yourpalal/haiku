//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _DISK_DEVICE_H
#define _DISK_DEVICE_H

#include <Partition.h>

struct user_disk_device_data;

class BDiskDevice : public BPartition {
public:
	BDiskDevice();
	virtual ~BDiskDevice();

	bool IsRemovable() const;
	bool HasMedia() const;

	status_t Eject(bool update = false);

	status_t Update(bool *updated = NULL);
	void Unset();

	virtual status_t GetPath(BPath *path) const;

	virtual BPartition *VisitEachDescendent(BDiskDeviceVisitor *visitor);
	
	bool IsModified() const;
	status_t PrepareModifications();
	status_t CommitModifications(bool synchronously = true,
		BMessenger progressMessenger = BMessenger(),
		bool receiveCompleteProgressUpdates = true);
	status_t CancelModifications();

private:
	friend class BDiskDeviceList;
	friend class BDiskDeviceRoster;

	status_t SetTo(partition_id id, size_t neededSize = 0);
	status_t SetTo(user_disk_device_data *data);

	user_disk_device_data	*fDeviceData;
};

#endif	// _DISK_DEVICE_H
