package gov.sandia.sst.oberon.compiler.visitor;

import gov.sandia.sst.oberon.compiler.exp.OberonExpressionException;
import gov.sandia.sst.oberon.compiler.stmt.*;

public interface OberonStatementBodyVisitor {

	void visit(OberonStatementBody oberonStatementBody)
			throws OberonStatementException, OberonExpressionException;
	void visit(ForWithAssignmentStatement forWithAssignmentStatement)
			throws OberonStatementException, OberonExpressionException;
	void visit(ForWithDeclarationStatement forWithDeclarationStatement)
			throws OberonStatementException, OberonExpressionException;
	void visit(FunctionDefinition functionDefinition) 
			throws OberonStatementException, OberonExpressionException;

}
