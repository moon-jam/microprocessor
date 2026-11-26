SHELL := /bin/bash

XC8 ?= /Applications/microchip/xc8/v4.00/bin/xc8-cc
DFP ?= $(HOME)/.mchp_packs/Microchip/PIC18Fxxxx_DFP/1.8.178/xc8
IPE_DIR ?= /Applications/microchip/mplabx/v6.20/mplab_platform/mplab_ipe
IPECMD ?= $(IPE_DIR)/bin/ipecmd.sh
BOOST_PORT ?= 2012
SCREEN ?= /usr/bin/screen
BAUD ?= 9600
TARGET_POWER ?= -W5.0

DEVICE := 18F4520

.PHONY: connect disconnect flash monitor clean
.DELETE_ON_ERROR:
export HEX PORT BAUD TARGET_POWER

out/main.hex: main.c ../../lib/pic18f4520_config.h \
              ../../lib/pic18f4520_hal.h
	mkdir -p out/.xc8
	cd out/.xc8 && "$(XC8)" -mcpu=$(DEVICE) "-mdfp=$(DFP)" -O1 "$(abspath $<)" -o main.elf
	mv out/.xc8/main.hex "$@"

clean:
	rm -rf out/.xc8 out/main.hex

define terminal_command
fail() { printf '%s\n' "$$*" >&2; return 1; }

tty_mode= tmp=
restore() {
    [[ -z "$$tmp" ]] || rm -r "$$tmp"
    [[ -z "$$tty_mode" ]] || stty "$$tty_mode"
}
trap restore EXIT
if [[ -t 0 && -t 2 ]]; then
    tty_mode=$$(stty -g)
    stty opost onlcr
fi

choose() {
    local missing="$$1" prompt="$$2" PS3=': ' COLUMNS=1
    shift 2
    case $$# in
        0) fail "$$missing" ;;
        1) selected=$$1 ;;
        *) printf '%s\n' "$$prompt" >&2
           select selected; do [[ -n "$$selected" ]] && return 0; done
           return 1 ;;
    esac
}

if [[ "$$0" == monitor ]]; then
    selected="$${PORT:-}"
    if [[ -z "$$selected" ]]; then
        shopt -s nullglob
        choose 'No USB serial ports found. Set PORT to select one explicitly.' 'Select a serial port' \
            /dev/cu.usbserial* /dev/cu.usbmodem* /dev/cu.wchusbserial* /dev/cu.SLAB_USBtoUART*
    fi
    [[ -c "$$selected" ]] || fail "Serial port not found: $$selected"
    "$(SCREEN)" "$$selected" "$$BAUD"
    exit 0
fi

port="$(BOOST_PORT)"
ipe=$$(cd "$(IPE_DIR)" && pwd -P)
state="$$HOME/.mchp_ipe/pic18f4520-$$port.session"
marker="$$HOME/.mchp_ipe/$$port.ini"
power="$$TARGET_POWER"

[[ "$$port" =~ ^[1-9][0-9]{0,4}$$ ]] && (( port <= 65535 )) || fail "Invalid Boost port: $$port"

valid_power() { [[ -z "$$1" || "$$1" =~ ^-W([0-9]+(\.[0-9]+)?)?$$ ]]; }
valid_power "$$power" || fail 'Set TARGET_POWER to a voltage option such as -W5.0, or leave it empty for external power.'

listener() { lsof -wanPt -iTCP:"$$port" -sTCP:LISTEN || :; }

is_boost() {
    [[ "$$1" =~ ^[0-9]+$$ && -e "$$marker" ]] &&
        [[ "$$(ps -ww -p "$$1" -o command=)" == *" -jar $$ipe/ipecmd.jar" ]]
}

boost() (
    . "$$ipe/jdkloc" && cd "$$ipe" || exit 1
    export PATH="$$ipe/../bin:$$PATH"
    exec /usr/bin/sandbox-exec -p '(version 1) (allow default) (deny network-inbound (remote ip "*:*")) (allow network-inbound (remote ip "localhost:*"))' \
        "$$jdkhome/bin/java" -jar ipecmdboost.jar "-OY$$port" "$$@"
)

check_ipe() {
    printf 'If MPLAB IPE is connected, click Disconnect or close MPLAB IPE, then try again.\n' >&2
}

disconnect_session() {
    [[ "$$(listener)" == "$$1" ]] && is_boost "$$1" || { fail 'The Boost session changed. Try again.'; return 1; }
    boost -OQ || :
    for ((attempt = 0; attempt < 20; ++attempt)); do
        if [[ "$$(listener)" != "$$1" && ! -e "$$marker" ]]; then
            rm -f -- "$$state"
            return 0
        fi
        sleep 0.1
    done
    fail "Boost did not disconnect from port $$port."
}

connect_session() {
    [[ -z "$$(listener)" && ! -e "$$marker" ]] ||
        { fail "Boost port $$port is in use or has stale state. Choose another BOOST_PORT."; return 1; }
    if ! boost -P$(DEVICE) -TPPK3 $${power:+"$$power"} -OK -OL; then
        local failed_pid=$$(listener)
        if is_boost "$$failed_pid"; then disconnect_session "$$failed_pid" || :; fi
        check_ipe
        return 1
    fi
    active=$$(listener)
    is_boost "$$active" || { fail "Could not identify the new Boost session on port $$port."; return 1; }
    if ! { printf '%s %s\n' "$$active" "$$power" > "$$state.tmp" &&
        mv -- "$$state.tmp" "$$state"; }; then
        rm -f -- "$$state.tmp"
        disconnect_session "$$active" || :
        return 1
    fi
    printf 'Boost session ready on port %s (%s).\n' "$$port" "$${power:-external power}"
}

active=
if [[ -r "$$state" ]] && read -r saved_pid saved_power < "$$state" &&
    [[ "$$(listener)" == "$$saved_pid" ]] && is_boost "$$saved_pid" && valid_power "$$saved_power"; then
    active=$$saved_pid
fi

case "$$0" in
    connect)
        if [[ -n "$$active" ]]; then
            printf 'Boost session already active on port %s (%s).\n' "$$port" "$${saved_power:-external power}"
        else
            connect_session
        fi
        ;;
    disconnect)
        if [[ -z "$$active" ]]; then
            printf 'No managed Boost session on port %s.\n' "$$port"
            exit 0
        fi
        printf 'Disconnect Boost on port %s? Target power may stop. [y/N]\n: ' "$$port" >&2
        read -r answer || { printf '\nCancelled.\n' >&2; exit 0; }
        case "$$answer" in
            y|Y|yes|YES) disconnect_session "$$active" ;;
            *) printf 'Cancelled.\n' ;;
        esac
        ;;
    flash)
        selected="$${HEX:-}"
        if [[ -z "$$selected" ]]; then
            items=()
            while IFS= read -r file; do items+=("$$file"); done \
                < <(find . -path './out/.xc8' -prune -o -type f -iname '*.hex' -print | LC_ALL=C sort)
            choose 'No .hex files found. Run make first.' 'Select a .hex file to flash' "$${items[@]}"
        fi
        tmp=$$(mktemp -d /tmp/pic18-flash.XXXXXX)
        cp -- "$$selected" "$$tmp/firmware.hex"
        program="$(IPECMD)"
        if [[ -n "$$active" ]]; then
            $(if $(filter command environment,$(origin TARGET_POWER)),:,power=$$saved_power)
            reuse="Reuse session ($${saved_power:-external power})"
            reconnect="Disconnect and reconnect ($${power:-external power})"
            items=("$$reconnect" "Cancel")
            if [[ "$$power" == "$$saved_power" ]]; then
                items=("$$reuse" "$${items[@]}")
            fi
            choose 'No session action available.' "Boost is active on port $$port. Select a flash action" "$${items[@]}"
            case "$$selected" in
                "$$reuse") ;;
                "$$reconnect") disconnect_session "$$active"; connect_session ;;
                *) printf 'Cancelled.\n'; exit 0 ;;
            esac
            [[ "$$(listener)" == "$$active" ]] && is_boost "$$active" || fail 'The Boost session changed. Try again.'
            program=boost
        else
            printf 'Tip: Run make connect before repeated flashes to reuse Boost and reduce startup time.\n'
        fi
        if ! "$$program" -P$(DEVICE) -TPPK3 $${power:+"$$power"} "-F$$tmp/firmware.hex" -M -Y -OL; then
            check_ipe
            exit 1
        fi
        ;;
esac
endef

export terminal_command
connect disconnect flash:
	@umask 077; mkdir -p "$$HOME/.mchp_ipe" && \
	    lockf -kt 0 "$$HOME/.mchp_ipe/pic18f4520.lock" "$(SHELL)" -ec "$$terminal_command" "$@"

monitor:
	@"$(SHELL)" -ec "$$terminal_command" "$@"
