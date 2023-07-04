# Throw an error with the given information.
export def throw [
    error: string,      # Error type
    message: string,    # Extra message
    meta: any,          # Metadata with span for the error hint
] {
    let span = $meta.span
    error make {
        msg: $error,
        label: {
            text: $message,
            start: $span.start,
            end: $span.end,
        },
    }
}

# Get root path of current git workspace.
export def git-ws [
    desired?: string, # Name of the desired workspace
] {
    mut current_dir = ($env.PWD | path expand)
    mut item = ($current_dir | path parse)
    loop {
        if ($current_dir | path join .git | path exists) {
            if ($desired | is-empty) or $item.stem == $desired {
                return $current_dir
            }
        }
        $current_dir = ($current_dir | path dirname)
        $item = ($current_dir | path parse)
        if ($item.parent | is-empty) {
            break
        }
    }
    let hint = if ($desired | is-empty) {
        'current directory is not in a git workspace'
    } else {
        $'can not find the desired workspace `($desired)`'
    }
    throw FileSystemError $hint (metadata $current_dir)
}

# Fuzzy search the full path of the executable.
export def "search executable" [
    executable: string,     # Incomplete pattern for the desired executable
    root_dir: string = '.', # Root directory for search
] {
    if ($root_dir | path type) != "dir" {
        throw IOError $"root path `($root_dir)` is not a directory" (metadata $root_dir)
    }
    let parent = ($executable | path dirname)
    let executable = if ($parent | is-empty) or $parent == '.' {
        $executable | path basename
    } else {
        $executable | path expand
    }
    let item = ($executable | path parse)
    let pattern = if ($item.extension | is-empty) {
        $'($item.stem)('(\.\w+)?$')'
    } else {
        $'($item.stem)\.($item.extension)$'
    }
    let result = (fd --search-path $root_dir -I -t x -S '+1kb' --regex $"'($pattern)'"
        | lines
        | filter {|e|
            let e = ($e | path expand | path parse)
            $item.stem == $e.stem and (($item.parent | is-empty) or $item.parent == $e.parent)
        }
        | par-each {|e|
            let path = ($e | path expand)
            let result = ($path | path parse)
            let dir = $result.parent
            let depth = ((($dir | path split) | length) - 1)
            let ext_length = ($result.extension | str length)
            let weight = ($depth * 1e6 + (($dir | str length) - $depth) * 1e3 + $ext_length)
            { path: $path, weight: $weight }
        }
        | sort-by weight
        )
    if not ($result | is-empty) {
        return ($result | first).path
    }
    throw IOError $"cannot find the target executable '($executable)'" (metadata $executable)
}

# Run profile on executables using llvm tool-set.
export def "run profile" [
    executable: string = 'slimec',      # Target executable for profile
    --sources: string = '',             # Input source files for profile
    --arguments (-v): string = '',      # Arguments for the executable
    --output (-o): string = 'profile',  # Name for the output file
    --open (-p),                        # Open the generated report
] {
    let pipe_data = (each {|e| $e | into string})
    let current_dir = $env.PWD
    let ws_dir = (git-ws)
    let test_dir = ($ws_dir | path join test)
    if ($test_dir | path type) != dir {
        throw IOError "no directory `test` under the current workspace" (metadata $ws_dir)
    }
    let executable = (path_to_executable $executable $'($ws_dir)/build')
    let name = ($executable | path parse).stem
    let profdata = $'($current_dir)\($name).profdata'
    let profhtml = $'($current_dir)\($output).html'
    let sources = if not ($sources | is-empty) {
        $sources
            | split row ','
            | uniq
            | where {|e| not ($e | is-empty) and ($e | path exists)}
            | str join (char nl)
    } else if ($pipe_data | describe) == 'nothing' {
        (fd --search-path $'($ws_dir)\src' -e cpp -e h)
    } else {
        $pipe_data
            | each {|e| $e | split row (char nl)}
            | flatten
            | each {|e| $e | split row ','}
            | flatten
            | uniq
            | where {|e| not ($e | is-empty) and ($e | path exists)}
            | str join (char nl)
    }
    do -p { nu -c $'^($executable) ($arguments)' } | complete | ignore
    llvm-profdata merge -o $profdata $'($current_dir)\default.profraw'
    llvm-cov show $executable $'-instr-profile=($profdata)' $sources -format html
        | save -f $profhtml
    if $open {
        start $profhtml
    }
}
