package gov.sandia.sst.oberon.compiler;

import java.util.Vector;

public final class OberonCompilerOptions {

	private static OberonCompilerOptions instance = null;
	private String outputPath;
	private boolean dumpASTToScreen;
	private boolean useMangledNames;
	private int bytesPerBoolean;
	
	private Vector<String> processFiles;
	
	private OberonCompilerOptions() {
		outputPath = "a.oba";
		dumpASTToScreen = false;
		useMangledNames = false;
		processFiles = new Vector<String>();
		bytesPerBoolean = 8;
	}
	
	public static OberonCompilerOptions getInstance() {
		if(instance == null) {
			instance = new OberonCompilerOptions();
		}
		
		return instance;
	}
	
	public void processOptions(String[] args) {
		for(int i = 0; i < args.length; i++) {
			if(args[i].startsWith("-")) {
				if(args[i].equals("-o")) {
					outputPath = args[i+1];
					i++;
				} else if (args[i].equals("-fdump")) {
					dumpASTToScreen = true;
				} else if (args[i].equals("-fbool-byte-alloc")) {
					bytesPerBoolean = Integer.parseInt(args[i+1]);
					i++;
				} else if (args[i].equals("-fuse-mangled-names")) {
					useMangledNames = true;
				}
			} else {
				// if not an option then we will try to compile it :)
				processFiles.add(args[i]);
			}
		}
	}
	
	// ------------------------------------------------------------
	
	public String getOutputPath() {
		return outputPath;
	}
	
	public int getBytesPerBoolean() {
		return bytesPerBoolean;
	}
	
	public Vector<String> getFilesForCompile() {
		return processFiles;
	}
	
	public boolean dumpASTToConsole() {
		return dumpASTToScreen;
	}
	
	public boolean useMangledNames() {
		return useMangledNames;
	}
}
