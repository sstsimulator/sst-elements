package gov.sandia.sst.oberon.compiler.exp;

public class OberonIncompatibleTypeException extends OberonExpressionException {

	public OberonIncompatibleTypeException(String fileName, int lineNo,
			int colNo, String message) {
		super(fileName, lineNo, colNo, message);
	}

}
