package neko.nme;

class Music
{
	static var __m;
	static var INFINITE_LOOP : Int = -1;
	
	public static function init( file : String )
	{
		__m = nme_music_init( untyped file.__s );
	}
	
	public static function free()
	{
		nme_music_free( __m );
	}
	
	public static function play( loops : Int )
	{
		// loops:
		// -1 = infinite
		if ( loops < -1 ) loops = -1;
		nme_music_play( __m, loops );
	}
	
	public static function fadeIn( loops : Int, fadeTimeMS : Int, position : Float )
	{
		// loops:
		// -1 = infinite
		// 0  = don't play
		// 1+ = number of loops
		if ( loops < -1 ) loops = -1;
		if ( position == 0 || position == null )
			nme_music_fadeinplay( __m, loops, fadeTimeMS );
		else
			nme_music_fadeinplaypos( __m, loops, fadeTimeMS, position );
	}
	
	public static function fadeOut( fadeTimeMS : Int )
	{
		if ( fadeTimeMS < 0 ) fadeTimeMS = Std.int( Math.abs( fadeTimeMS ) );
		nme_music_fadeout( fadeTimeMS );
	}
	
	public static function stop()
	{
		nme_music_stop();
	}
	
	public static function pause()
	{
		nme_music_pause();
	}
	
	public static function resume()
	{
		nme_music_resume();
	}
	
	public static function restart()
	{
		nme_music_restart();
	}
	
	public static function setVolume( vol : Int )
	{
		if ( vol < 0 ) vol = 0;
		if ( vol > 128 ) vol = 128;
		nme_music_volume( vol );
	}
	
	public static function isPlaying() : Bool
	{
		return ( nme_music_isplaying() == 1 );
	}
	
	public static function isPaused() : Bool
	{
		return ( nme_music_ispaused() == 1 );
	}
	
	public static function isFading() : Bool
	{
		return ( nme_music_isfading() == 1 );
	}
	
	static var nme_music_init = neko.Lib.load("nme","nme_music_init", 1);
	static var nme_music_free = neko.Lib.load("nme","nme_music_free", 1);
	static var nme_music_play = neko.Lib.load("nme","nme_music_play", 2);
	static var nme_music_fadeinplay = neko.Lib.load("nme","nme_music_fadeinplay", 3);
	static var nme_music_fadeinplaypos = neko.Lib.load("nme","nme_music_fadeinplaypos", 4);
	static var nme_music_fadeout = neko.Lib.load("nme","nme_music_fadeout", 1);
	static var nme_music_stop = neko.Lib.load("nme","nme_music_stop", 0);
	static var nme_music_pause = neko.Lib.load("nme","nme_music_pause", 0);
	static var nme_music_resume = neko.Lib.load("nme","nme_music_resume", 0);
	static var nme_music_restart = neko.Lib.load("nme","nme_music_restart", 0);
	static var nme_music_volume = neko.Lib.load("nme","nme_music_volume", 1);
	static var nme_music_isplaying = neko.Lib.load("nme","nme_music_isplaying", 0);
	static var nme_music_ispaused = neko.Lib.load("nme","nme_music_ispaused", 0);
	static var nme_music_isfading = neko.Lib.load("nme","nme_music_isfading", 0);
}