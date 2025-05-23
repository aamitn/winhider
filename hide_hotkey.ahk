#NoTrayIcon  ; Hide tray icon, comment this to show tray icon

; If either executable is built locally, use that instead
; FileExist and SetWorkingDir syntax changed
if FileExist(".\Build\bin\Release\Winhider.exe") || FileExist(".\Build\bin\Release\Winhider_32bit.exe")
    SetWorkingDir A_ScriptDir "\Build\bin\Release" ; Use A_ScriptDir to ensure path is relative to the script

; Function to determine if the process is 32-bit
Is32BitProcess(pid) {
    ; DllCall syntax changed
    hProcess := DllCall("kernel32\OpenProcess", "UInt", 0x1000, "Int", 0, "UInt", pid, "Ptr")
    if !hProcess
        return false  ; Assume 64-bit if we can't open the process

    isWow64 := 0
    ; DllCall syntax changed, output parameter is different
    success := DllCall("kernel32\IsWow64Process", "Ptr", hProcess, "Int*", isWow64)
    ; DllCall syntax changed
    DllCall("kernel32\CloseHandle", "Ptr", hProcess)

    if !success
        return false  ; Assume 64-bit if call fails

    return isWow64  ; Returns 1 if 32-bit, 0 if 64-bit
}

; Runs the correct Winhider variant with the appropriate argument
RunWinhiderCommand(command) {
    ; WinGet syntax changed
    pid := WinGetPID("A")
    if !pid {
        MsgBox "Could not get PID of active window." ; MsgBox syntax changed
        return
    }

    if (Is32BitProcess(pid)) {
        exe := "Winhider_32bit.exe"
    } else {
        exe := "Winhider.exe"
    }

    ; Run command syntax changed, options are now an object
    Run exe " --" command " " pid,, "Hide"
}

; Hotkeys syntax changed
^h:: { 
    RunWinhiderCommand("hide")
}

^j:: {
    RunWinhiderCommand("unhide")
}

^k:: {
    RunWinhiderCommand("hidetask")
}

^l:: {
    RunWinhiderCommand("unhidetask")
}

^q:: {
    ExitApp ; ExitApp syntax is the same
}
