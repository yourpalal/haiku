/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler, haiku@clemens-zeidler.de
 * Distributed under the terms of the MIT License.
 */
#ifndef	LINEAR_SPEC_H
#define	LINEAR_SPEC_H

#include <math.h>

#include <List.h>
#include <OS.h>
#include <String.h>
#include <SupportDefs.h>

#include "Constraint.h"
#include "OperatorType.h"
#include "OptimizationType.h"
#include "PenaltyFunction.h"
#include "ResultType.h"
#include "Summand.h"
#include "Variable.h"

#include "lp_lib.h"


namespace LinearProgramming {

/*!
 * Specification of a linear programming problem.
 */
class LinearSpec {
public:
								LinearSpec();
	virtual						~LinearSpec();

			Variable*			AddVariable();

			Constraint*			AddConstraint(SummandList* summands,
									OperatorType op, double rightSide);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									OperatorType op, double rightSide);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									OperatorType op, double rightSide);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									double coeff3, Variable* var3,
									OperatorType op, double rightSide);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									double coeff3, Variable* var3,
									double coeff4, Variable* var4,
									OperatorType op, double rightSide);

			Constraint*			AddConstraint(SummandList* summands,
									OperatorType op, double rightSide,
									double penaltyNeg, double penaltyPos);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									OperatorType op, double rightSide,
									double penaltyNeg, double penaltyPos);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									OperatorType op, double rightSide,
									double penaltyNeg, double penaltyPos);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									double coeff3, Variable* var3,
									OperatorType op, double rightSide,
									double penaltyNeg, double penaltyPos);
			Constraint*			AddConstraint(double coeff1, Variable* var1,
									double coeff2, Variable* var2,
									double coeff3, Variable* var3,
									double coeff4, Variable* var4,
									OperatorType op, double rightSide,
									double penaltyNeg, double penaltyPos);

			PenaltyFunction*	AddPenaltyFunction(Variable* var, BList* xs,
									BList* gs);

			SummandList*		ObjFunction();
			void				SetObjFunction(SummandList* summands);
			void				UpdateObjFunction();

			ResultType			Presolve();
			void				RemovePresolved();
			ResultType			Solve();
			void				Save(const char* fileName);

			int32				CountColumns() const;
			OptimizationType	Optimization() const;
			void				SetOptimization(OptimizationType value);

			ResultType			Result() const;
			double				ObjectiveValue() const;
			double				SolvingTime() const;

			operator BString() const;
			void				GetString(BString& string) const;

protected:
			VariableList*		Variables() const;
			ConstraintList*		Constraints() const;

			int32 				fCountColumns;

private:
			lprec*				fLpPresolved;
			OptimizationType	fOptimization;
			lprec*				fLP;
			SummandList*		fObjFunction;
			VariableList		fVariables;
			ConstraintList		fConstraints;
			ResultType			fResult;
			double 				fObjectiveValue;
			double 				fSolvingTime;

public:
	friend class		Constraint;
	friend class		Variable;

};

}	// namespace LinearProgramming

using LinearProgramming::LinearSpec;

#endif	// LINEAR_SPEC_H
