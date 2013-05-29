package gov.sandia.sst.oberon.compiler.stmt;

import gov.sandia.sst.oberon.compiler.exp.OberonExpression;
import gov.sandia.sst.oberon.compiler.exp.OberonExpressionException;
import gov.sandia.sst.oberon.compiler.visitor.OberonVisitor;

import java.util.*;

public class CallStatement extends OberonStatement {

	protected String funcName;
	protected Vector<OberonExpression> parameters;
	
	public CallStatement(String fileName, int lineno, int colno,
			String functionName, Vector<OberonExpression> funcParams) {
		super(fileName, lineno, colno);
		funcName = functionName;
		parameters = funcParams;
	}
	
	public String getFunctionName() {
		return funcName;
	}
	
	public Vector<OberonExpression> getFunctionParameters() {
		return parameters;
	}

	public void processVisitor(OberonVisitor visit) throws 
		OberonStatementException, OberonExpressionException {
		visit.visit(this);
	}

	public int increaseAllocationByBytes() {
		return 0;
	}

}
