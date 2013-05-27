package gov.sandia.sst.oberon.compiler.visitor;

import gov.sandia.sst.oberon.compiler.exp.OberonExpressionException;
import gov.sandia.sst.oberon.compiler.stmt.*;

public interface OberonVisitor {

	void visit(AssignmentStatement assignmentStatement) throws 
		OberonStatementException, OberonExpressionException;
	void visit(DeclarationStatement declarationStatement) throws 
	OberonStatementException, OberonExpressionException;
	void visit(ForWithAssignmentStatement forWithAssignmentStatement) throws 
	OberonStatementException, OberonExpressionException;
	void visit(ForWithDeclarationStatement forWithDeclarationStatement) throws 
	OberonStatementException, OberonExpressionException;
	void visit(IfElseStatement ifElseStatement) throws 
	OberonStatementException, OberonExpressionException;
	void visit(ReturnStatement returnStatement) throws 
		OberonStatementException, OberonExpressionException;
	void visit(CallStatement callStatement)  throws 
	OberonStatementException, OberonExpressionException;
	
}
