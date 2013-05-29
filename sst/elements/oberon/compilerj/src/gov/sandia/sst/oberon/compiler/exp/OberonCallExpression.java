package gov.sandia.sst.oberon.compiler.exp;

import java.util.Vector;

import gov.sandia.sst.oberon.compiler.OberonFunctionTable;
import gov.sandia.sst.oberon.compiler.OberonNameMangler;
import gov.sandia.sst.oberon.compiler.stmt.OberonStatementException;
import gov.sandia.sst.oberon.compiler.visitor.OberonExpressionVisitor;

public class OberonCallExpression extends OberonExpression {

	protected String calleeName;
	protected Vector<OberonExpression> parameters;
	
	public OberonCallExpression(String fileName, int lineNo, int colNo,
			String callee, Vector<OberonExpression> params) {
		super(fileName, lineNo, colNo);
		
		calleeName = callee;
		parameters = params;
	}
	
	public void processVisitorTarget(OberonExpressionVisitor expVisit)
			throws OberonStatementException, OberonExpressionException {
		expVisit.visit(this);
	}

	public OberonVariableType getExpressionType()
			throws OberonIncompatibleTypeException {
		
		String mangleName = calleeName + 
				OberonNameMangler.getMangleAddition(fileName, lineNo, colNo, parameters);
		
		return OberonFunctionTable.getInstance().getFunctionType(fileName, lineNo, colNo, mangleName);
	}
	
	public String getCalleeName() {
		return calleeName;
	}
	
	public String getCalleeMangledName() throws OberonIncompatibleTypeException {
		String mangleName = calleeName + 
				OberonNameMangler.getMangleAddition(fileName, lineNo, colNo, parameters);
		return mangleName;
	}
	
	public Vector<OberonExpression> getParameters() {
		return parameters;
	}

}
