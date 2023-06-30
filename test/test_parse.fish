#!/usr/bin/fish
set pass_cnt 0
set unpass_cnt 0

function handle_sigsegv
    echo "Recieve SIGSEGV in test\n"
end

for file in (ls ./functional/*.sy)
    # echo $(realpath $file)
    trap handle_sigsegv SIGSEGV
    set output (slimec $(realpath $file) 2>> /dev/null)
    if test $status -eq 0
        set pass_cnt (math "$pass_cnt + 1")
    else
        set unpass_cnt (math "$unpass_cnt + 1")
        set msg $file "not pass."
        echo $msg
    end
end
set_color green
echo "Pass" $pass_cnt "tests"
set_color red
echo $unpass_cnt not pass.