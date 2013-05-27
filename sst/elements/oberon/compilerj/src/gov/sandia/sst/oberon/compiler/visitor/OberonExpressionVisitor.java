package gov.sandia.sst.oberon.compiler.visitor;

import gov.sandia.sst.oberon.compiler.exp.*;
import gov.sandia.sst.oberon.compiler.stmt.OberonStatementException;

public interface OberonExpressionVisitor {

	void visit(OberonAddExpression oberonAddExpression)
			 throws 
				OberonStatementException, OberonExpressionException;
	void visit(OberonBooleanLiteral oberonBooleanLiteral)
			 throws 
				OberonStatementException, OberonExpressionException;
	void visit(OberonDivideExpression oberonDivideExpression)
			 throws 
				OberonStatementException, OberonExpressionException;
	void visit(OberonDoubleLiteral oberonDoubleLiteral)
			 throws 
				OberonStatementException, OberonExpressionException;
	void visit(OberonEqualityExpression oberonEqualityExpression)
			 throws 
				OberonStatementException, OberonExpressionException;
	void visit(OberonGreaterThanEqualExpression oberonGreaterThanEqualExpression)
			 throws 
				OberonStatementException, OberonExpressionException;
	void visit(OberonGreaterThanExpression oberonGreaterThanExpression) throws 
	OberonStatementException, OberonExpressionException;
	void visit(OberonIntegerLiteral oberonIntegerLiteral) throws 
	OberonStatementException, OberonExpressionException;
	void visit(OberonLessThanEqualExpression oberonLessThanEqualExpression) throws 
	OberonStatementException, OberonExpressionException;
	void visit(OberonLessThanExpression oberonLessThanExpression) throws 
	OberonStatementException, OberonExpressionException;
	void visit(OberonMultiplyExpression oberonMultiplyExpression) throws 
	OberonStatementException, OberonExpressionException;
	void visit(OberonNotEqualityExpression oberonNotEqualityExpression) throws 
	OberonStatementException, OberonExpressionException;
	void visit(OberonStringLiteral oberonStringLiteral) throws 
	OberonStatementException, OberonExpressionException;
	void visit(OberonSubExpression oberonSubExpression) throws 
	OberonStatementException, OberonExpressionException;
	void visit(OberonVariableExpression oberonVariableExpression) throws 
	OberonStatementException, OberonExpressionException;
	void visit(OberonOrExpression oberonOrExpression) throws 
	OberonStatementException, OberonExpressionException;
	void visit(OberonAndExpression oberonAndExpression) throws 
	OberonStatementException, OberonExpressionException;
	void visit(OberonBracketedExpression oberonBracketedExpression) throws 
	OberonStatementException, OberonExpressionException;

}
