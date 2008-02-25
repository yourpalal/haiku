/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#ifndef	VARIABLE_H
#define	VARIABLE_H

#include <SupportDefs.h>


namespace LinearProgramming {

class Constraint;
class LinearSpec;

/**
 * Contains minimum and maximum values.
 */
class Variable {

public:
	int32				Index();
	LinearSpec*			LS() const;
	void				SetLS(LinearSpec* value);
	double				Value() const;
	void				SetValue(double value);
	double				Min() const;
	void				SetMin(double min);
	double				Max() const;
	void				SetMax(double max);
	void				SetRange(double min, double max);
	//~ string			ToString();
	Constraint*			IsEqual(Variable* var);
	Constraint*			IsSmallerOrEqual(Variable* var);
	Constraint*			IsGreaterorEqual(Variable* var);

protected:
						Variable(LinearSpec* ls);
						~Variable();

private:
	LinearSpec*			fLS;
	double				fValue;
	double				fMin;
	double				fMax;

public:
	friend class			LinearSpec;
	friend class			Constraint;

};

}	// namespace LinearProgramming

using LinearProgramming::Variable;

#endif	// VARIABLE_H
