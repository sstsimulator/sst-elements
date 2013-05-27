package gov.sandia.sst.oberon.compiler.exp;

public abstract class OberonLiteralExpression extends OberonUnaryExpression {

	public OberonLiteralExpression(String fileName, int lineNo, int colNo,
			OberonExpression expr) {
		super(fileName, lineNo, colNo, expr);
	}

}
