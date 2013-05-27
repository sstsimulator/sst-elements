package gov.sandia.sst.oberon.compiler.visitor;

import gov.sandia.sst.oberon.compiler.exp.*;
import gov.sandia.sst.oberon.compiler.stmt.*;

import java.io.*;
import java.util.Vector;

public class OberonASTPrinter implements OberonExpressionVisitor,
		OberonVisitor, OberonStatementBodyVisitor {

	protected PrintStream os;
	protected int currentLevel;
	protected final String perLevelString;
	
	public OberonASTPrinter(PrintStream output) {
		os = output;
		currentLevel = 0;
		perLevelString = "  ";
		os.println();
	}

	public void visit(AssignmentStatement assignmentStatement) throws OberonStatementException, OberonExpressionException {
		printCurrentLineIndent();
		
		os.print(assignmentStatement.getVariableName());
		os.print(" = ");
		
		assignmentStatement.getAssignmentValue().processVisitorTarget(this);
		
		os.println(";");
	}

	private void printCurrentLineIndent() {
		for(int i = 0; i < currentLevel; i++) {
			System.out.print(perLevelString);
		}
	}

	public void visit(DeclarationStatement declarationStatement) throws OberonStatementException, OberonExpressionException {
		printCurrentLineIndent();
		
		os.print(convertTypeToString(declarationStatement.getVariableType()));
		os.print(" ");
		os.print(declarationStatement.getVariableName());
		
		if(declarationStatement.getInitialAssignmentExpression() != null) {
			os.print(" = ");
			declarationStatement.getInitialAssignmentExpression().processVisitorTarget(this);
		}
		
		os.println(";");
	}

	public void visit(ForWithAssignmentStatement forWithAssignmentStatement) throws OberonStatementException, OberonExpressionException {
		printCurrentLineIndent();
		
		os.print("for(");
		forWithAssignmentStatement.getAssignmentStatement().processVisitor(this);
		os.print("; ");
		forWithAssignmentStatement.getLoopCondition().processVisitorTarget(this);
		os.print("; ");
		forWithAssignmentStatement.getIncrementStatement().processVisitor(this);
		os.println(") {");
		
		currentLevel++;
		for(OberonStatement stmt : forWithAssignmentStatement.getStatementBody().getStatements()) {
			stmt.processVisitor(this);
		}
		currentLevel--;
		
		printCurrentLineIndent();
		os.println("}");
	}

	public void visit(ForWithDeclarationStatement forWithDeclarationStatement) throws OberonStatementException, OberonExpressionException {
		printCurrentLineIndent();
		
		os.print("for(");
		//forWithDeclarationStatement.getDeclarationStatement().processVisitor(this);
		DeclarationStatement forDecl = forWithDeclarationStatement.getDeclarationStatement();
		os.print(this.convertTypeToString(forDecl.getVariableType()) + " " + 
				forDecl.getVariableName() + " = ");
		forDecl.getInitialAssignmentExpression().processVisitorTarget(this);
		os.print("; ");
		forWithDeclarationStatement.getLoopCondition().processVisitorTarget(this);
		os.print("; ");
		AssignmentStatement assignStmt = forWithDeclarationStatement.getIncrementStatement();
		if(assignStmt != null) {
			os.print(assignStmt.getVariableName() + " = ");
			assignStmt.getAssignmentValue().processVisitorTarget(this);
		}
		os.println(") {");
		
		currentLevel++;
		for(OberonStatement stmt : forWithDeclarationStatement.getStatementBody().getStatements()) {
			stmt.processVisitor(this);
		}
		currentLevel--;
		
		printCurrentLineIndent();
		os.println("}");
	}
	
	public void visit(IfElseStatement ifElseStatement) throws OberonStatementException, OberonExpressionException {
		printCurrentLineIndent();
		
		os.print("if(");
		ifElseStatement.getCondition().processVisitorTarget(this);
		os.println(") {");
		
		currentLevel++;
		for(OberonStatement stmt : ifElseStatement.getIfStatements().getStatements()) {
			stmt.processVisitor(this);
		}
		currentLevel--;
		
		printCurrentLineIndent();
		
		if(ifElseStatement.getElseStatements().getStatements().size() > 0) {
			os.print("} else {");
			currentLevel++;
			for(OberonStatement stmt : ifElseStatement.getElseStatements().getStatements()) {
				stmt.processVisitor(this);
			}
			currentLevel--;
			
			printCurrentLineIndent();
			os.println("}");
		} else {
			os.println("}");
		}
	}

	public void visit(ReturnStatement returnStatement) throws OberonStatementException, OberonExpressionException {
		printCurrentLineIndent();
		
		os.print("return ");
		returnStatement.getReturnExpression().processVisitorTarget(this);
		os.println(";");
	}

	public void visit(CallStatement callStatement) throws OberonStatementException, OberonExpressionException {
		printCurrentLineIndent();
		
		os.print(callStatement.getFunctionName() + "(");
		
		Vector<OberonExpression> params = callStatement.getFunctionParameters();
		for(int i = 0; i < params.size(); i++) {
			params.get(i).processVisitorTarget(this);
			
			if(i <= (params.size() - 1)) {
				os.print(", ");
			}
		}
		
		os.println(")");
	}

	public void visit(OberonAddExpression oberonAddExpression) throws OberonStatementException, OberonExpressionException {
		oberonAddExpression.getLeft().processVisitorTarget(this);
		os.print(" + ");
		oberonAddExpression.getRight().processVisitorTarget(this);
	}

	public void visit(OberonBooleanLiteral oberonBooleanLiteral) {
		if(oberonBooleanLiteral.getBooleanValue()) {
			os.print("TRUE");
		} else {
			os.print("FALSE");
		}
	}

	public void visit(OberonDivideExpression oberonDivideExpression) throws OberonStatementException, OberonExpressionException {
		oberonDivideExpression.getLeft().processVisitorTarget(this);
		os.print(" / ");
		oberonDivideExpression.getRight().processVisitorTarget(this);
	}

	public void visit(OberonDoubleLiteral oberonDoubleLiteral) {
		os.print(oberonDoubleLiteral.getDoubleValue());
	}

	public void visit(OberonEqualityExpression oberonEqualityExpression) throws OberonStatementException, OberonExpressionException {
		oberonEqualityExpression.getLeft().processVisitorTarget(this);
		os.print(" == ");
		oberonEqualityExpression.getRight().processVisitorTarget(this);
	}

	public void visit(OberonGreaterThanEqualExpression oberonGreaterThanEqualExpression) throws OberonStatementException, OberonExpressionException {
		oberonGreaterThanEqualExpression.getLeft().processVisitorTarget(this);
		os.print(" >= ");
		oberonGreaterThanEqualExpression.getRight().processVisitorTarget(this);
	}

	public void visit(OberonGreaterThanExpression oberonGreaterThanExpression) throws OberonStatementException, OberonExpressionException {
		oberonGreaterThanExpression.getLeft().processVisitorTarget(this);
		os.print(" > ");
		oberonGreaterThanExpression.getRight().processVisitorTarget(this);
	}

	public void visit(OberonIntegerLiteral oberonIntegerLiteral) {
		os.print(oberonIntegerLiteral.getIntegerValue());
	}

	public void visit(OberonLessThanEqualExpression oberonLessThanEqualExpression) throws OberonStatementException, OberonExpressionException {
		oberonLessThanEqualExpression.processVisitorTarget(this);
		os.print(" <= ");
		oberonLessThanEqualExpression.getRight().processVisitorTarget(this);
	}

	public void visit(OberonLessThanExpression oberonLessThanExpression) throws OberonStatementException, OberonExpressionException {
		oberonLessThanExpression.getLeft().processVisitorTarget(this);
		os.print(" < ");
		oberonLessThanExpression.getRight().processVisitorTarget(this);
	}

	public void visit(OberonMultiplyExpression oberonMultiplyExpression) throws OberonStatementException, OberonExpressionException {
		oberonMultiplyExpression.getLeft().processVisitorTarget(this);
		os.print(" * ");
		oberonMultiplyExpression.getRight().processVisitorTarget(this);
	}

	public void visit(OberonNotEqualityExpression oberonNotEqualityExpression) throws OberonStatementException, OberonExpressionException {
		oberonNotEqualityExpression.getLeft().processVisitorTarget(this);
		os.print(" != ");
		oberonNotEqualityExpression.getRight().processVisitorTarget(this);
	}

	public void visit(OberonStringLiteral oberonStringLiteral) {
		os.print("\"" + oberonStringLiteral.getStringValue() + "\"");
	}

	public void visit(OberonSubExpression oberonSubExpression) throws OberonStatementException, OberonExpressionException {
		oberonSubExpression.getLeft().processVisitorTarget(this);
		os.print(" - ");
		oberonSubExpression.getRight().processVisitorTarget(this);
	}

	public void visit(OberonVariableExpression oberonVariableExpression) {
		os.print(oberonVariableExpression.getVariableName());
	}

	public void visit(OberonOrExpression oberonOrExpression) throws OberonStatementException, OberonExpressionException {
		oberonOrExpression.getLeft().processVisitorTarget(this);
		os.print(" || ");
		oberonOrExpression.getRight().processVisitorTarget(this);
	}

	public void visit(OberonAndExpression oberonAndExpression) throws OberonStatementException, OberonExpressionException {
		oberonAndExpression.getLeft().processVisitorTarget(this);
		os.print(" && ");
		oberonAndExpression.getRight().processVisitorTarget(this);
	}

	public void visit(OberonBracketedExpression oberonBracketedExpression) throws OberonStatementException, OberonExpressionException {
		os.print("( ");
		oberonBracketedExpression.processVisitorTarget(this);
		os.print(" )");
	}

	public void visit(OberonStatementBody oberonStatementBody) throws OberonStatementException, OberonExpressionException {
		for(OberonStatement nextStmt : oberonStatementBody.getStatements() ) {
			printCurrentLineIndent();
			
			nextStmt.processVisitor(this);
			
			os.println();
		}
	}

	public void visit(FunctionDefinition functionDefinition) throws 
		OberonStatementException, OberonExpressionException {
		
		os.println("// " + functionDefinition.getFileName() +
				" @ Line: " + functionDefinition.getLineNumber() +
				", Column: " + functionDefinition.getColumnNumber());
		os.println("// Stack: " + functionDefinition.getDeclaredVariableSize() + " bytes");
		os.print(convertTypeToString(functionDefinition.getFunctionType()));
		os.print(" ");
		os.print(functionDefinition.getFunctionName());
		os.print("(");
		
		Vector<TypeNamePair> params = functionDefinition.getParameters();
		
		for(int i = 0; i < params.size(); i++) {
			os.print(convertTypeToString(params.get(i).getType()));
			os.print(" ");
			os.print(params.get(i).getName());
			
			if(i < params.size() - 1) {
				os.print(", ");
			}
		}
		
		os.println(") {");
		currentLevel++;
		
		for(OberonStatement stmt : functionDefinition.getStatements()) {
			stmt.processVisitor(this);
		}
		
		currentLevel--;
		os.println("}");
		os.println();
	}

	public String convertTypeToString(OberonVariableType type) {
		switch(type) {
		case DOUBLE:
			return "double";

		case INTEGER:
			return "int";

		case BOOLEAN:
			return "bool";

		case STRING:
			return "string";
			
		case VOID:
			return "void";
		}
		
		return "Unknown";
	}
	
}
