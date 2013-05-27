package gov.sandia.sst.oberon.compiler;

public final class OberonCompilerOptions {

	private static OberonCompilerOptions instance = null;
	private String outputPath;
	
	private OberonCompilerOptions() {
		outputPath = "output.obn";
	}
	
	public static OberonCompilerOptions getInstance() {
		if(instance == null) {
			instance = new OberonCompilerOptions();
		}
		
		return instance;
	}
	
	// ------------------------------------------------------------
	
	public String getOutputPath() {
		return outputPath;
	}

}
