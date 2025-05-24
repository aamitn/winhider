; Start with the tray icon hidden
A_IconHidden := true
; Initialize a variable to track the tray icon's state.
; 0 means hidden, 1 means visible.
trayIconVisible := 0

; If either executable is built locally, use that instead
; FileExist and SetWorkingDir syntax changed
if FileExist(A_ScriptDir "\Build\bin\Release\Winhider.exe") || FileExist(A_ScriptDir "\Build\bin\Release\Winhider_32bit.exe") {
    SetWorkingDir A_ScriptDir "\Build\bin\Release" ; Use A_ScriptDir to ensure path is relative to the script
}

; Function to determine if a process is 32-bit (improved version)
IsProcess32Bit(pid) {
    try {
        ; Open process handle with PROCESS_QUERY_INFORMATION access
        hProcess := DllCall("OpenProcess", "UInt", 0x0400, "Int", false, "UInt", pid, "Ptr")
        
        if (!hProcess) {
            ; If we can't open the process, make an educated guess
            ; On 32-bit OS, all processes are 32-bit
            return !A_Is64bitOS
        }
        
        ; Check if process is running under WOW64 (32-bit on 64-bit OS)
        isWow64 := 0
        result := DllCall("IsWow64Process", "Ptr", hProcess, "Int*", &isWow64)
        
        ; Close process handle
        DllCall("CloseHandle", "Ptr", hProcess)
        
        if (!result) {
            ; If IsWow64Process fails, fallback to OS-based assumption
            return !A_Is64bitOS
        }
        
        ; If running on 64-bit OS
        if (A_Is64bitOS) {
            ; If process is under WOW64, it's 32-bit
            ; If not under WOW64, it's 64-bit
            return isWow64
        } else {
            ; On 32-bit OS, all processes are 32-bit
            return true
        }
        
    } catch Error as e {
        ; Fallback: assume based on OS architecture
        return !A_Is64bitOS
    }
}

; Runs the correct Winhider variant with the appropriate argument
RunWinhiderCommand(command) {
    ; WinGet syntax changed
    try {
        pid := WinGetPID("A")
        if (!pid) {
            MsgBox("Could not get PID of active window.", "Error", "IconX")
            return
        }
        
        ; Get process name for better error reporting
        processName := WinGetProcessName("A")
        
        ; Determine which executable to use
        if(IsProcess32Bit(pid)) {
            exe := "Winhider_32bit.exe"
            architecture := "32-bit"
        } else {
            exe := "Winhider.exe"
            architecture := "64-bit"
        }
        
        ; Check if the executable exists
        if (!FileExist(exe)) {
            MsgBox("Could not find " . exe . "`n`nPlease ensure the executable is in the correct directory.", "File Not Found", "IconX")
            return
        }
        
        ; Show brief status (optional - remove if you don't want notifications)
        ; ToolTip("Running " . exe . " for " . processName . " (" . architecture . ")")
        ; SetTimer(() => ToolTip(), -1500)
        
        ; Run command syntax changed, options are now an object
        Run(exe . " --" . command . " " . pid, , "Hide")
        
    } catch Error as e {
        MsgBox("Error executing command: " . e.Message, "Error", "IconX")
    }
}

; Hotkeys syntax changed
^+h:: { 
    RunWinhiderCommand("hide")
}

^+j:: {
    RunWinhiderCommand("unhide")
}

^+k:: {
    RunWinhiderCommand("hidetask")
}

^+l:: {
    RunWinhiderCommand("unhidetask")
}

^+q:: {
    ExitApp ; ExitApp syntax is the same
}

; Define a hotkey to toggle the tray icon (e.g., F1)
^F10:: {
    global trayIconVisible
    if (trayIconVisible = 0) {
        A_IconHidden := false ; Show the tray icon
        trayIconVisible := 1 ; Update our state variable
        MsgBox("Tray icon is now visible.", "AutoHotkey Script")
    } else {
        A_IconHidden := true ; Hide the tray icon
        trayIconVisible := 0 ; Update our state variable
        MsgBox("Tray icon is now hidden.", "AutoHotkey Script")
    }
}

; Optional: Hotkey to show current active window process info (for debugging)
^+i:: {
    try {
        pid := WinGetPID("A")
        processName := WinGetProcessName("A")
        windowTitle := WinGetTitle("A")
        is32Bit := IsProcess32Bit(pid)
        architecture := is32Bit ? "32-bit" : "64-bit"
        
        info := "Active Window Information:`n`n"
        info .= "Title: " . windowTitle . "`n"
        info .= "Process: " . processName . "`n"
        info .= "PID: " . pid . "`n"
        info .= "Architecture: " . architecture . "`n"
        info .= "Will use: " . (is32Bit ? "Winhider_32bit.exe" : "Winhider.exe")
        
        MsgBox(info, "Process Info", "T10")
        
    } catch Error as e {
        MsgBox("Error getting process info: " . e.Message, "Error", "IconX")
    }
}