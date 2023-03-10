#!/usr/bin/env nu

# 基础文法解析测试

def main [SSYC_EXECUTABLE: string] {
	let TEST_DIR = ($env.FILE_PWD | path join)
	let SAMPLES = (ls $'($TEST_DIR)/functional/*.sy' | get name)

	let total = ($SAMPLES | length)
	mut failed = 0

	echo $'[============] Running ($total) test from 1 test case'
	echo
	
	for test_in in $SAMPLES {
		echo '[------------]'
		echo $'[ RUN        ] ($test_in)'

		let resp = (do { ^$SSYC_EXECUTABLE --input $test_in } | complete)
		# echo $resp.stderr

		echo '[         OK ]'
		
		if $resp.exit_code != 0 {
			$failed = $failed + 1
			echo '[------------] failed'
		} else {
			echo '[------------] passed'
		}

		echo
	}

	echo $'[============] ($total) test from 1 test case ran'
	if $failed == 0 {
		echo $'[   PASSED   ] ($total) test'
	} else {
		echo $'[   FAILED   ] ($failed) test'
	}
}
