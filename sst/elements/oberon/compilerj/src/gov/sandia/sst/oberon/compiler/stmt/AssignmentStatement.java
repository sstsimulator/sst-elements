package gov.sandia.sst.oberon.compiler.stmt;

import gov.sandia.sst.oberon.compiler.exp.OberonExpression;
import gov.sandia.sst.oberon.compiler.exp.OberonExpressionException;
import gov.sandia.sst.oberon.compiler.visitor.OberonVisitor;

public class AssignmentStatement extends OberonStatement {

	protected String variableName;
	protected OberonExpression assignmentValue;
	
	public AssignmentStatement(int lineno, int colno,
			String variableName, 
			OberonExpression assignVal) {
		super(lineno, colno);
		
		this.variableName = variableName;
		this.assignmentValue = assignVal;
	}
	
	public OberonExpression getAssignmentValue() {
		return assignmentValue;
	}
	
	public String getVariableName() {
		return variableName;
	}

	public int increaseAllocationByBytes() {
		return 0;
	}

	public void processVisitor(OberonVisitor visit)
			throws OberonStatementException, OberonExpressionException {

		visit.visit(this);
	}

}
