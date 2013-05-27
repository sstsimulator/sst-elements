package gov.sandia.sst.oberon.compiler.stmt;

import gov.sandia.sst.oberon.compiler.visitor.*;
import java.util.Vector;

public interface StatementBlock extends OberonStatementBodyVisitorTarget {

	public abstract void addStatement(OberonStatement stmt);
	public abstract Vector<OberonStatement> getStatements();
	public abstract int getDeclaredVariableSize();
	
}
