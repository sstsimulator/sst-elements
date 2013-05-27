package gov.sandia.sst.oberon.compiler.stmt;

import java.util.Vector;

public interface StatementBlock {

	public abstract void addStatement(OberonStatement stmt);
	public abstract Vector<OberonStatement> getStatements();
	public abstract int getDeclaredVariableSize();
	
}
