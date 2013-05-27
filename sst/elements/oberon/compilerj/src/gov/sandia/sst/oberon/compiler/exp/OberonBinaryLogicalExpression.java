package gov.sandia.sst.oberon.compiler.exp;

public abstract class OberonBinaryLogicalExpression extends OberonBinaryExpression {

	public OberonBinaryLogicalExpression(
			String fileName,
			int lineNo,
			int colNo,
			OberonExpression onLeft,
			OberonExpression onRight) {
		super(fileName, lineNo, colNo, onLeft, onRight);
	}

	public OberonVariableType getExpressionType() {
		return OberonVariableType.BOOLEAN;
	}

}
