package;


import neko.Lib;
import nme.Loader;


class LogHelper {
	
	
	public static var mute:Bool;
	public static var verbose:Bool = false;
	
	private static var sentWarnings:Hash <Bool> = new Hash <Bool> ();
	
	
	public static function error (message:String, verboseMessage:String = "", e:Dynamic = null):Void {
		
		if (message != "") {
			
			try {
				
				if (verbose && verboseMessage != "") {
					
					nme_error_output ("Error: " + verboseMessage + "\n");
					
				} else {
					
					nme_error_output ("Error: " + message + "\n");
					
				}
				
			} catch (e:Dynamic) {}
			
		}
		
		if (verbose && e != null) {
			
			Lib.rethrow (e);
			
		}
		
		Sys.exit (1);
		
	}
	
	
	public static function info (message:String, verboseMessage:String = ""):Void {
		
		if (verbose && verboseMessage != "") {
			
			Sys.println (verboseMessage);
			
		} else if (message != "") {
			
			Sys.println (message);
			
		}
		
	}
	
	
	public static function warn (message:String, verboseMessage:String = "", allowRepeat:Bool = false):Void {
		
		var output = "";
		
		if (verbose && verboseMessage != "") {
			
			output = "Warning: " + verboseMessage;
			
		} else if (message != "") {
			
			output = "Warning: " + message;
			
		}
		
		if (!allowRepeat && sentWarnings.exists (output)) {
			
			return;
			
		}
		
		sentWarnings.set (output, true);
		Sys.println (output);
		
	}
	
	
	private static var nme_error_output = Loader.load ("nme_error_output", 1);
		

}
