#!/bin/bash
# Falkon-OS Build Wrapper Script
# Provides a simplified shell interface to the Python build.py engine
# Compatible with bash, sh, and POSIX-compliant shells

# Set strict error handling
if [ -n "${DEBUG:-}" ]; then
    set -x
fi
set -e

# Configuration
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_SCRIPT="${ROOT_DIR}/build.py"

# Display help information
function show_help() {
    cat <<EOM
Falkon-OS Build Wrapper Script v1.0

Usage: build.sh [OPTIONS]

Options:
  -h, --help              Show this help message
  -clean [target]         Clean build artifacts
  -build [profile]        Build kernel (profile: dev, debug, release, etc.)
  -run [mode]             Launch in VM (mode: vbox, qemu, normal, debug, serial)
  -check                  Check build environment and dependencies
  -test                   Run automated test suite

Profile Options (used with -build):
  dev     Debug mode with symbols (default)
  debug   Deep debug mode
  release Optimized for size/speed
  asan    Address sanitizer enabled
  ubsan   Undefined behavior sanitizer enabled

Mode Options (used with -run):
  vbox    VirtualBox (default)
  qemu    QEMU emulator
  normal  Normal QEMU boot
  debug   QEMU with GDB debugging
  serial  QEMU with serial console

Examples:
  build.sh -check                    Check environment
  build.sh -build                   Build in dev mode
  build.sh -build release            Build optimized release
  build.sh -run vbox                 Build and launch in VirtualBox
  build.sh -clean all                Clean all build artifacts
EOM
}

# Clean build artifacts
function run_clean() {
    local target="${1:-all}"

    echo "🧹 Cleaning Falkon-OS build artifacts (target: ${target})..."

    # Call build.py with clean subcommand
    python3 "${BUILD_SCRIPT}" -clean "${target}"
}

# Build the kernel
function run_build() {
    local profile="${1:-dev}"
    local save_mode="false"
    local force_rebuild="false"

    echo "🔧 Building Falkon-OS kernel (profile: ${profile})..."

    # Parse additional options
    while [[ $# -gt 0 ]]; do
        case "${1}" in
            --save)
                save_mode="true"
                shift
                ;;
            --force|-f)
                force_rebuild="true"
                shift
                ;;
            *)
                break
                ;;
        esac
    done

    # Build command arguments
    local build_args="${profile}"
    if [[ "${save_mode}" == "true" ]]; then
        build_args="${build_args} -save"
    fi
    if [[ "${force_rebuild}" == "true" ]]; then
        build_args="${build_args} -f"
    fi

    # Call build.py with build subcommand
    python3 "${BUILD_SCRIPT}" -build ${build_args}
}

# Run the VM
function run_vm() {
    local mode="${1:-normal}"

    echo "🚀 Launching Falkon-OS VM (mode: ${mode})..."

    # Call build.py with run subcommand
    python3 "${BUILD_SCRIPT}" -run "${mode}"
}

# Check build environment
function run_check() {
    echo "🔍 Checking Falkon-OS build environment..."
    python3 "${BUILD_SCRIPT}" -check
}

# Run automated test suite
function run_tests() {
    echo "🧪 Running Falkon-OS automated test suite..."
    python3 "${BUILD_SCRIPT}" -test
}

# Main execution
function main() {
    # Check if Python script exists
    if [[ ! -f "${BUILD_SCRIPT}" ]]; then
        echo "❌ Error: build.py not found at ${BUILD_SCRIPT}"
        exit 1
    fi

    # Parse command line arguments
    if [[ $# -eq 0 ]]; then
        show_help
        exit 0
    fi

    local cmd="${1}"
    shift

    case "${cmd}" in
        -h|--help)
            show_help
            ;;
        -clean)
            run_clean "${1:-all}"
            ;;
        -build)
            run_build "${1:-dev}" "$@"
            ;;
        -run)
            run_vm "${1:-normal}"
            ;;
        -check)
            run_check
            ;;
        -test)
            run_tests
            ;;
        *)
            echo "❌ Unknown command: ${cmd}"
            show_help
            exit 1
            ;;
    esac
}

# Execute main function with all arguments
main "$@"
