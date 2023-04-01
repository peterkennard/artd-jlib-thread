myDir = File.dirname(File.expand_path(__FILE__));
require "#{myDir}/../build-options.rb";

module Rakish

Rakish::CppProject.new(
	:name 			=> "artd-jlib-thread",
	:package 		=> "artd",
	:dependsUpon 	=> [ '../artd-lib-logger',
	                     '../artd-jlib-base'
	                   ]
) do

	cppDefine('BUILDING_artd_jlib_thread');
		
	addPublicIncludes('include/artd/*.h');
	addPublicIncludes('include/artd/thread/*', :destdir=>'artd/thread');

    addSourceFiles(
        './Semaphore.cpp',
        './StallMonitor.cpp',
        './WaitableSignal.cpp',
        './OsThread.cpp',
        './Thread.cpp'
    );

    setupCppConfig :targetType =>'DLL' do |cfg|
    end
end

end # module Rakish

