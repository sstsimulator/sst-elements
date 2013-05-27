package gov.sandia.sst.oberon.compiler.exp;

public abstract class OberonBinaryMathematicalExpression extends
		OberonBinaryExpression {

	public OberonBinaryMathematicalExpression(
			String fileName,
			int lineNo,
			int colNo,
			OberonExpression onLeft,
			OberonExpression onRight) {
		super(fileName, lineNo, colNo, onLeft, onRight);
	}

	public OberonVariableType getExpressionType() throws OberonIncompatibleTypeException {
		if(left.getExpressionType() == OberonVariableType.BOOLEAN) {
			throw new OberonIncompatibleTypeException(left.getFileName(),
					left.getLineNumber(), left.getColumnNumber(),
				"Attempted to perform a mathemtical operation on a boolean value");
		} else if (right.getExpressionType() == OberonVariableType.BOOLEAN) {
			throw new OberonIncompatibleTypeException(left.getFileName(),
					left.getLineNumber(), left.getColumnNumber(),
				"Attempted to perform a mathematical operation on a boolean value");
		} else if(left.getExpressionType() == OberonVariableType.STRING ||
					right.getExpressionType() == OberonVariableType.STRING) {
			throw new OberonIncompatibleTypeException(left.getFileName(),
					left.getLineNumber(), left.getColumnNumber(),
				"Attempted to perform a mathematical operation on a string");
		} else if(left.getExpressionType() == OberonVariableType.DOUBLE) {
			return OberonVariableType.DOUBLE;
		} else if(right.getExpressionType() == OberonVariableType.DOUBLE) {
			return OberonVariableType.DOUBLE;
		} else {
			return OberonVariableType.INTEGER;
		}
	}
	
}
