#!/usr/bin/fish
set pass_cnt 0
set unpass_cnt 0
set libfunc_test 0
set libfunc getint putint getarray putarray putch
set msg "null"


function handle_sigsegv
    echo "Recieve SIGSEGV in test\n"
end

function check_content
    rg $argv[1] $argv[2] -l >> /dev/null
    if test $status -eq 0
        set msg $argv[2] "not pass with "$argv[1]"()"
    end
end

for file in (ls test/functional/*.sy)
    # echo $(realpath $file)
    trap handle_sigsegv SIGSEGV
    set output (slimec $(realpath $file) 2>> /dev/null)
    if test $status -eq 0
        set pass_cnt (math "$pass_cnt + 1")
    else
        set unpass_cnt (math "$unpass_cnt + 1")
        set msg "null"
        for func in $libfunc
            check_content $func $file
        end
        if test "$msg" = "null"
            set msg $file "not pass".
        else
            set libfunc_test (math $libfunc_test + 1)
        end
        
        echo $msg
    end

end
set_color green
echo "Pass" $pass_cnt "tests"
set_color red
echo $unpass_cnt not pass.
set_color yellow
echo "NR of tests with libfunction: $libfunc_test" #之前统计过了，哈哈
