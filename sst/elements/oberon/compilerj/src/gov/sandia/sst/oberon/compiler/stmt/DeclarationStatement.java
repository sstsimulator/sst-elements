package gov.sandia.sst.oberon.compiler.stmt;

import gov.sandia.sst.oberon.compiler.exp.OberonExpression;
import gov.sandia.sst.oberon.compiler.exp.OberonExpressionException;
import gov.sandia.sst.oberon.compiler.exp.OberonVariableType;
import gov.sandia.sst.oberon.compiler.visitor.OberonVisitor;

public class DeclarationStatement extends OberonStatement {

	protected OberonExpression initialAssignment;
	protected OberonVariableType varType;
	protected String variableName;
	
	public DeclarationStatement(String fileName, int lineno, int colno,
			String varName, OberonVariableType theType,
			OberonExpression initAssignment) {
		super(fileName, lineno, colno);

		varType = theType;
		variableName = varName;
		initialAssignment = initAssignment;
	}
	
	public OberonExpression getInitialAssignmentExpression() {
		return initialAssignment;
	}
	
	public String getVariableName() {
		return variableName;
	}
	
	public OberonVariableType getVariableType() {
		return varType;
	}

	public int increaseAllocationByBytes() {
		int incSize = 0;
		
		switch( varType ) {
		case INTEGER:
		case DOUBLE:
			incSize = 8;
			break;
		case BOOLEAN:
			incSize = 4;
			break;
		case STRING:
			// need to do something here
			break;
		}
		
		return incSize;
	}

	public void processVisitor(OberonVisitor visit)
			throws OberonStatementException, OberonExpressionException {

		visit.visit(this);
	}

}
