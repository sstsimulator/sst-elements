package gov.sandia.sst.oberon.compiler.visitor;

import gov.sandia.sst.oberon.compiler.exp.OberonExpressionException;
import gov.sandia.sst.oberon.compiler.stmt.OberonStatementException;

public interface OberonVisitorTarget {

	public void processVisitor(OberonVisitor visit) throws
		OberonStatementException, OberonExpressionException;
	
}
