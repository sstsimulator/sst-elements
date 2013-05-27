package gov.sandia.sst.oberon.compiler.exp;

public class OberonExpressionException extends Exception {

	protected int lineNo;
	protected int colNo;
	protected String fileName;
	
	public OberonExpressionException(String fileName,
			int lineNo, int colNo) {
		
		this.fileName = fileName;
		this.lineNo = lineNo;
		this.colNo = colNo;
	}

	public OberonExpressionException(String fileName,
			int lineNo, int colNo, String message) {
		super(message);
		
		this.fileName = fileName;
		this.lineNo = lineNo;
		this.colNo = colNo;
	}

	public OberonExpressionException(String fileName,
			int lineNo, int colNo, Throwable cause) {
		super(cause);
		
		this.fileName = fileName;
		this.lineNo = lineNo;
		this.colNo = colNo;
	}

	public OberonExpressionException(String fileName,
			int lineNo, int colNo, String message, Throwable cause) {
		super(message, cause);
		
		this.fileName = fileName;
		this.lineNo = lineNo;
		this.colNo = colNo;
	}

	public OberonExpressionException(String fileName,
			int lineNo, int colNo, String message, Throwable cause,
			boolean enableSuppression, boolean writableStackTrace) {
		super(message, cause, enableSuppression, writableStackTrace);
		
		this.fileName = fileName;
		this.lineNo = lineNo;
		this.colNo = colNo;
	}

	public String getFileName() {
		return fileName;
	}
	
	public int getLineNumber() {
		return lineNo;
	}
	
	public int getColumnNumber() {
		return colNo;
	}
	
}
