package gov.sandia.sst.oberon.compiler.visitor;

import gov.sandia.sst.oberon.compiler.exp.*;
import gov.sandia.sst.oberon.compiler.stmt.*;


public interface OberonExpressionVisitorTarget {

	public void processVisitorTarget(OberonExpressionVisitor expVisit) throws
		OberonStatementException, OberonExpressionException;
	
}
