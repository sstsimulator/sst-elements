package gov.sandia.sst.oberon.compiler.visitor;

import gov.sandia.sst.oberon.compiler.exp.*;

public interface OberonExpressionVisitor {

	void visit(OberonAddExpression oberonAddExpression);
	void visit(OberonBooleanLiteral oberonBooleanLiteral);
	void visit(OberonDivideExpression oberonDivideExpression);
	void visit(OberonDoubleLiteral oberonDoubleLiteral);
	void visit(OberonEqualityExpression oberonEqualityExpression);
	void visit(OberonGreaterThanEqualExpression oberonGreaterThanEqualExpression);
	void visit(OberonGreaterThanExpression oberonGreaterThanExpression);
	void visit(OberonIntegerLiteral oberonIntegerLiteral);
	void visit(OberonLessThanEqualExpression oberonLessThanEqualExpression);
	void visit(OberonLessThanExpression oberonLessThanExpression);
	void visit(OberonMultiplyExpression oberonMultiplyExpression);
	void visit(OberonNotEqualityExpression oberonNotEqualityExpression);
	void visit(OberonStringLiteral oberonStringLiteral);
	void visit(OberonSubExpression oberonSubExpression);
	void visit(OberonVariableExpression oberonVariableExpression);

}
