package gov.sandia.sst.oberon.compiler.visitor;

import gov.sandia.sst.oberon.compiler.exp.OberonExpressionException;
import gov.sandia.sst.oberon.compiler.stmt.OberonStatementException;

public interface OberonStatementBodyVisitorTarget {

	public void visit(OberonStatementBodyVisitor visit) throws
		OberonStatementException, OberonExpressionException;
	
}
