package gov.sandia.sst.oberon.compiler.visitor;

import java.io.*;

import gov.sandia.sst.oberon.compiler.OberonNameMangler;
import gov.sandia.sst.oberon.compiler.exp.*;
import gov.sandia.sst.oberon.compiler.stmt.*;

public class OberonAssemblyPrinter implements OberonExpressionVisitor,
		OberonStatementBodyVisitor, OberonVisitor {

	protected PrintStream os;
	protected int currentLevel;
	protected static int currentLabel = 0;
	
	public OberonAssemblyPrinter(PrintStream output) {
		os = output;
		currentLevel = 0;
	}
	
	public int getCurrentLabel() {
		return currentLabel;
	}
	
	public void incrementLabel() {
		currentLabel++;
	}
	
	public int getCurrentLabelAndIncrement() {
		int label = currentLabel;
		currentLabel++;
		
		return label;
	}
	
	public void printLevelIndent() {
		for(int i = 0; i < currentLevel; i++) {
			os.print("  ");
		}
	}

	public void visit(AssignmentStatement assignmentStatement)
			throws OberonStatementException, OberonExpressionException {

		assignmentStatement.getAssignmentValue().processVisitorTarget(this);
		
		printLevelIndent();
		os.println("pop " + assignmentStatement.getVariableName());
	}

	public void visit(DeclarationStatement declarationStatement)
			throws OberonStatementException, OberonExpressionException {
		
	}

	
	public void visit(IfElseStatement ifElseStatement)
			throws OberonStatementException, OberonExpressionException {
		os.println("L" + getCurrentLabelAndIncrement() + ":");

		ifElseStatement.getCondition().processVisitorTarget(this);
		// need a jump here
		os.println("L" + getCurrentLabelAndIncrement() + ":");
		
		for(OberonStatement nxtStmt : ifElseStatement.getIfStatements().getStatements()) {
			nxtStmt.processVisitor(this);
		}
		
		// need a jump here
		os.println("L" + getCurrentLabelAndIncrement() + ":");
		
		for(OberonStatement nxtStmt : ifElseStatement.getElseStatements().getStatements()) {
			nxtStmt.processVisitor(this);
		}
		
		os.println("L" + getCurrentLabelAndIncrement() + ":");
	}

	public void visit(ReturnStatement returnStatement)
			throws OberonStatementException, OberonExpressionException {
		
		returnStatement.getReturnExpression().processVisitorTarget(this);
		printLevelIndent();
		os.println("return");
	}

	public void visit(CallStatement callStatement)
			throws OberonStatementException, OberonExpressionException {
		
		for(OberonExpression expr : callStatement.getFunctionParameters()) {
			expr.processVisitorTarget(this);
		}
		
		printLevelIndent();
		os.println("call " + callStatement.getFunctionName());
	}

	public void visit(OberonStatementBody oberonStatementBody)
			throws OberonStatementException, OberonExpressionException {
		
		for(OberonStatement nxtStmt : oberonStatementBody.getStatements()) {
			nxtStmt.processVisitor(this);
		}
	}

	public void visit(ForWithAssignmentStatement forWithAssignmentStatement)
			throws OberonStatementException, OberonExpressionException {
		
		forWithAssignmentStatement.getAssignmentStatement().processVisitor(this);
		os.println("L" + getCurrentLabelAndIncrement() + ":");
		
		forWithAssignmentStatement.getLoopCondition().processVisitorTarget(this);
		
		for(OberonStatement nxtStmt : forWithAssignmentStatement.getStatementBody().getStatements()) {
			nxtStmt.processVisitor(this);
		}
		
		forWithAssignmentStatement.getIncrementStatement().processVisitor(this);
		
		// need a jump here.
	}

	public void visit(ForWithDeclarationStatement forWithDeclarationStatement)
			throws OberonStatementException, OberonExpressionException {
		
		forWithDeclarationStatement.getDeclarationStatement().processVisitor(this);
		os.println("L" + getCurrentLabelAndIncrement() + ":");
		
		forWithDeclarationStatement.getLoopCondition().processVisitorTarget(this);
		for(OberonStatement nxtStmt : forWithDeclarationStatement.getStatementBody().getStatements()) {
			nxtStmt.processVisitor(this);
		}
		
		forWithDeclarationStatement.getIncrementStatement().processVisitor(this);
		
		// need a jump here.
	}

	public void visit(FunctionDefinition functionDefinition)
			throws OberonStatementException, OberonExpressionException {
		
		os.print("_" + functionDefinition.getFunctionName());
		os.println(OberonNameMangler.getMangleAdditionByTypes(functionDefinition.getFileName(),
				functionDefinition.getLineNumber(),
				functionDefinition.getColumnNumber(), 
				functionDefinition.getParameters()) + ":");
		
		currentLevel++;
		
		for(OberonStatement nxtStmt : functionDefinition.getStatements()) {
			nxtStmt.processVisitor(this);
		}
		
		os.println();
		printLevelIndent();
		os.println("# Automatic Function Return");
		printLevelIndent();
		os.println("return");
		os.println();
		currentLevel--;
		os.println();
	}

	public void visit(OberonAddExpression oberonAddExpression)
			throws OberonStatementException, OberonExpressionException {
		
		oberonAddExpression.getRight().processVisitorTarget(this);
		oberonAddExpression.getLeft().processVisitorTarget(this);
		printLevelIndent();
		os.println("add");
	}

	public void visit(OberonBooleanLiteral oberonBooleanLiteral)
			throws OberonStatementException, OberonExpressionException {
		
		printLevelIndent();
		os.println("push_boolean");
		printLevelIndent();
		
		if(oberonBooleanLiteral.getBooleanValue()) {
			os.println("true");
		} else {
			os.println("false");
		}
	}

	public void visit(OberonDivideExpression oberonDivideExpression)
			throws OberonStatementException, OberonExpressionException {
		
		oberonDivideExpression.getRight().processVisitorTarget(this);
		oberonDivideExpression.getLeft().processVisitorTarget(this);
		printLevelIndent();
		os.println("div");
	}

	public void visit(OberonDoubleLiteral oberonDoubleLiteral)
			throws OberonStatementException, OberonExpressionException {
		
		printLevelIndent();
		os.println("push_double");
		printLevelIndent();
		os.println("" + oberonDoubleLiteral.getDoubleValue());
	}

	public void visit(OberonEqualityExpression oberonEqualityExpression)
			throws OberonStatementException, OberonExpressionException {
		
		oberonEqualityExpression.getRight().processVisitorTarget(this);
		oberonEqualityExpression.getLeft().processVisitorTarget(this);
		printLevelIndent();
		os.println("eq");
	}

	public void visit(
			OberonGreaterThanEqualExpression oberonGreaterThanEqualExpression)
			throws OberonStatementException, OberonExpressionException {
		
		oberonGreaterThanEqualExpression.getRight().processVisitorTarget(this);
		oberonGreaterThanEqualExpression.getLeft().processVisitorTarget(this);
		printLevelIndent();
		os.println("gte");
	}

	public void visit(OberonGreaterThanExpression oberonGreaterThanExpression)
			throws OberonStatementException, OberonExpressionException {
		
		oberonGreaterThanExpression.getRight().processVisitorTarget(this);
		oberonGreaterThanExpression.getLeft().processVisitorTarget(this);
		printLevelIndent();
		os.println("gt");
	}

	public void visit(OberonIntegerLiteral oberonIntegerLiteral)
			throws OberonStatementException, OberonExpressionException {
		
		printLevelIndent();
		os.println("push_integer");
		printLevelIndent();
		os.println("" + oberonIntegerLiteral.getIntegerValue());
	}

	public void visit(
			OberonLessThanEqualExpression oberonLessThanEqualExpression)
			throws OberonStatementException, OberonExpressionException {
		
		oberonLessThanEqualExpression.getRight().processVisitorTarget(this);
		oberonLessThanEqualExpression.getLeft().processVisitorTarget(this);
		printLevelIndent();
		os.println("lte");
	}

	public void visit(OberonLessThanExpression oberonLessThanExpression)
			throws OberonStatementException, OberonExpressionException {
		
		oberonLessThanExpression.getRight().processVisitorTarget(this);
		oberonLessThanExpression.getLeft().processVisitorTarget(this);
		printLevelIndent();
		os.println("lt");
	}

	public void visit(OberonMultiplyExpression oberonMultiplyExpression)
			throws OberonStatementException, OberonExpressionException {
		
		oberonMultiplyExpression.getRight().processVisitorTarget(this);
		oberonMultiplyExpression.getLeft().processVisitorTarget(this);
		printLevelIndent();
		os.println("mul");
	}

	public void visit(OberonNotEqualityExpression oberonNotEqualityExpression)
			throws OberonStatementException, OberonExpressionException {
		
		oberonNotEqualityExpression.getRight().processVisitorTarget(this);
		oberonNotEqualityExpression.getLeft().processVisitorTarget(this);
		printLevelIndent();
		os.println("neq");
	}

	public void visit(OberonStringLiteral oberonStringLiteral)
			throws OberonStatementException, OberonExpressionException {
		
		printLevelIndent();
		os.println("push_string " + (oberonStringLiteral.getStringValue().length() + 1));
		printLevelIndent();
		os.println("\"" + oberonStringLiteral.getStringValue() + "\"");
	}

	public void visit(OberonSubExpression oberonSubExpression)
			throws OberonStatementException, OberonExpressionException {
		
		oberonSubExpression.getRight().processVisitorTarget(this);
		oberonSubExpression.getLeft().processVisitorTarget(this);
		printLevelIndent();
		os.println("sub");
	}

	public void visit(OberonVariableExpression oberonVariableExpression)
			throws OberonStatementException, OberonExpressionException {
		
		printLevelIndent();
		os.println("push " + oberonVariableExpression.getVariableName());
	}

	public void visit(OberonOrExpression oberonOrExpression)
			throws OberonStatementException, OberonExpressionException {
		oberonOrExpression.getRight().processVisitorTarget(this);
		oberonOrExpression.getLeft().processVisitorTarget(this);
		printLevelIndent();
		os.println("or");
	}

	public void visit(OberonAndExpression oberonAndExpression)
			throws OberonStatementException, OberonExpressionException {
		oberonAndExpression.getRight().processVisitorTarget(this);
		oberonAndExpression.getLeft().processVisitorTarget(this);
		printLevelIndent();
		os.println("and");
	}

	public void visit(OberonBracketedExpression oberonBracketedExpression)
			throws OberonStatementException, OberonExpressionException {
		
		oberonBracketedExpression.getExpression().processVisitorTarget(this);
	}

	public void visit(OberonCallExpression oberonCallExpression)
			throws OberonStatementException, OberonExpressionException {
		
		printLevelIndent();
		os.println("call " + oberonCallExpression.getCalleeMangledName());
	}

}
