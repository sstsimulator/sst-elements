package gov.sandia.sst.oberon.compiler.exp;

public abstract class OberonBinaryExpression extends OberonExpression {

	protected OberonExpression left;
	protected OberonExpression right;
	
	public OberonBinaryExpression(
			String fileName, int lineNo, int colNo,
			OberonExpression onLeft,
			OberonExpression onRight) {
		
		super(fileName, lineNo, colNo);
		
		left = onLeft;
		right = onRight;
	}
	
	public OberonExpression getLeft() {
		return left;
	}
	
	public OberonExpression getRight() {
		return right;
	}
	
}
