package gov.sandia.sst.oberon.compiler.exp;

public abstract class OberonUnaryExpression extends OberonExpression {

	protected OberonExpression theExpr;
	
	public OberonUnaryExpression(String fileName,
			int lineNo,
			int colNo,
			OberonExpression expr) {
		
		super(fileName, lineNo, colNo);
		theExpr = expr;
	}
	
	public OberonExpression getExpression() {
		return theExpr;
	}

}
