package gov.sandia.sst.oberon.compiler.stmt;

public class OberonStatementException extends Exception {

	protected String fileName;
	protected int lineNo;
	protected int colNo;
	
	public OberonStatementException(String fileName,
			int lineNo,
			int colNo,
			String message) {
		super(message);
		
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
