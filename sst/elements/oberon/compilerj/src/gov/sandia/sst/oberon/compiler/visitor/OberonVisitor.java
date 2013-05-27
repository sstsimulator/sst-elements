package gov.sandia.sst.oberon.compiler.visitor;

import gov.sandia.sst.oberon.compiler.stmt.*;

public interface OberonVisitor {

	void visit(AssignmentStatement assignmentStatement);
	void visit(DeclarationStatement declarationStatement);
	void visit(ForWithAssignmentStatement forWithAssignmentStatement);
	void visit(ForWithDeclarationStatement forWithDeclarationStatement);
	void visit(IfElseStatement ifElseStatement);
	void visit(ReturnStatement returnStatement);
	void visit(CallStatement callStatement);
	
}
