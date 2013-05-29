package gov.sandia.sst.oberon.compiler.stmt;

import gov.sandia.sst.oberon.compiler.exp.OberonExpressionException;
import gov.sandia.sst.oberon.compiler.visitor.OberonStatementBodyVisitor;

import java.util.Vector;

public class OberonStatementBody implements StatementBlock {

	protected Vector<OberonStatement> statements;
	
	public OberonStatementBody() {
		statements = new Vector<OberonStatement>();
	}
	
	public void addStatement(OberonStatement stmt) {
		statements.add(stmt);
	}

	public Vector<OberonStatement> getStatements() {
		return statements;
	}

	public int getDeclaredVariableSize() {
		return 0;
	}

	public void visit(OberonStatementBodyVisitor visit) throws 
		OberonStatementException, OberonExpressionException {
		visit.visit(this);
	}

}
