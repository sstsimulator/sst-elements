package gov.sandia.sst.oberon.compiler.stmt;

import gov.sandia.sst.oberon.compiler.OberonCompilerOptions;
import gov.sandia.sst.oberon.compiler.OberonVariableSizeEstimator;
import gov.sandia.sst.oberon.compiler.exp.OberonExpression;
import gov.sandia.sst.oberon.compiler.exp.OberonExpressionException;
import gov.sandia.sst.oberon.compiler.exp.OberonIncompatibleTypeException;
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

	public int increaseAllocationByBytes() throws OberonIncompatibleTypeException {
		return OberonVariableSizeEstimator.OberonVariableSizeEstimator(fileName,
				lineno, colno, varType);
	}

	public void processVisitor(OberonVisitor visit) throws 
	OberonStatementException, OberonExpressionException {

		visit.visit(this);
	}

}
