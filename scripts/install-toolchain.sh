#!/usr/bin/env bash
# install-toolchain.sh
# Install LumenOS compiler toolchain from toolchain.json
# Usage: bash scripts/install-toolchain.sh --ubuntu
#        bash scripts/install-toolchain.sh --verify
#        bash scripts/install-toolchain.sh --help

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TOOLCHAIN_FILE="$PROJECT_ROOT/toolchain.json"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() { echo -e "${BLUE}ℹ${NC} $*"; }
log_success() { echo -e "${GREEN}✓${NC} $*"; }
log_warn() { echo -e "${YELLOW}⚠${NC} $*"; }
log_error() { echo -e "${RED}✗${NC} $*" >&2; }

# Parse toolchain.json
parse_json() {
    local key="$1"
    python3 -c "import json; d=json.load(open('$TOOLCHAIN_FILE')); print(d$key)" 2>/dev/null
}

# Check if command exists with specific version
check_version() {
    local cmd="$1"
    local min_version="$2"
    
    if ! command -v "$cmd" &> /dev/null; then
        return 1
    fi
    return 0
}

# Display help
show_help() {
    cat <<EOF
${BLUE}LumenOS Toolchain Installation${NC}

Usage: bash scripts/install-toolchain.sh [COMMAND]

Commands:
  --ubuntu      Install on Ubuntu/Debian systems
  --verify      Verify installed toolchain matches pinned versions
  --versions    Display all pinned versions from toolchain.json
  --help        Show this message

Examples:
  bash scripts/install-toolchain.sh --ubuntu
  bash scripts/install-toolchain.sh --verify

Note:
  For full reproducibility, use Docker instead:
    docker build -f Dockerfile -t lumenos-build .
    docker run -it -v \$(pwd):/workspace lumenos-build bash

EOF
}

# Install on Ubuntu/Debian
install_ubuntu() {
    log_info "Installing LumenOS toolchain for Ubuntu/Debian..."
    
    # Check if running on Ubuntu/Debian
    if ! [[ -f /etc/os-release ]]; then
        log_error "Cannot detect OS. This script requires /etc/os-release"
        exit 1
    fi
    
    source /etc/os-release
    if [[ ! " ubuntu debian " =~ " $ID " ]]; then
        log_warn "This script is optimized for Ubuntu/Debian. Your distro is: $ID"
        log_warn "Attempting anyway, but may not work..."
        read -p "Continue? (y/n) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    fi
    
    log_info "Step 1: Add LLVM repository..."
    if ! grep -q "llvm-toolchain" /etc/apt/sources.list.d/* 2>/dev/null; then
        if ! command -v add-apt-repository &> /dev/null; then
            log_info "Installing software-properties-common..."
            sudo apt-get update -y
            sudo apt-get install -y software-properties-common
        fi
        
        log_info "Adding LLVM apt repository..."
        curl https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
        
        # Detect Ubuntu codename
        CODENAME=$(lsb_release -cs)
        sudo add-apt-repository "deb http://apt.llvm.org/$CODENAME/ llvm-toolchain-$CODENAME-18 main"
    else
        log_success "LLVM repository already configured"
    fi
    
    log_info "Step 2: Update package lists..."
    sudo apt-get update -y
    
    log_info "Step 3: Install LLVM 18 toolchain..."
    sudo apt-get install -y \
        clang-18 \
        llvm-18 \
        lld-18 \
        llvm-18-tools
    
    log_info "Step 4: Install assembler and build tools..."
    sudo apt-get install -y \
        nasm \
        make \
        pkg-config \
        build-essential
    
    log_info "Step 5: Create symbolic links for default names..."
    for tool in clang clang++ llc llvm-ar llvm-objcopy llvm-objdump; do
        if [[ -f /usr/bin/${tool}-18 ]]; then
            sudo ln -sfn /usr/bin/${tool}-18 /usr/bin/$tool || true
        fi
    done
    if [[ -f /usr/bin/lld-18 ]]; then
        sudo ln -sfn /usr/bin/lld-18 /usr/bin/ld.lld || true
    fi
    
    log_info "Step 6: Install QEMU and OVMF (for testing)..."
    sudo apt-get install -y \
        qemu-system-x86 \
        ovmf
    
    log_info "Step 7: Install GDB (optional, for debugging)..."
    sudo apt-get install -y gdb || log_warn "GDB installation failed (optional, continuing...)"
    
    log_info "Step 8: Install Rust (optional, for userspace)..."
    if ! command -v rustc &> /dev/null; then
        curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- \
            --default-toolchain 1.75.0 \
            -y
        source "$HOME/.cargo/env"
    else
        log_warn "Rust already installed, skipping (use 'rustup update 1.75.0' to pin)"
    fi
    
    log_success "Toolchain installation complete!"
    log_info "\nSetting environment variables..."
    
    # Create env file
    cat > "$PROJECT_ROOT/.toolchain.env" <<'ENVFILE'
# LumenOS Toolchain Environment Variables
# Source this before building: source .toolchain.env
export CC=clang-18
export CXX=clang++-18
export LD=ld.lld-18
export AR=llvm-ar-18
export OBJCOPY=llvm-objcopy-18
export OBJDUMP=llvm-objdump-18
ENVFILE
    
    log_success "Environment file created: .toolchain.env"
    log_info "Source it with: source .toolchain.env\n"
    
    # Verify installation
    log_info "Verifying toolchain..."
    verify_toolchain
}

# Verify installed toolchain
verify_toolchain() {
    log_info "Verifying toolchain components...\n"
    
    local failed=0
    
    # Check LLVM/Clang
    if command -v clang-18 &> /dev/null; then
        log_success "clang-18: $(clang-18 --version | head -1)"
    else
        log_error "clang-18 not found"
        failed=1
    fi
    
    # Check LLD
    if command -v ld.lld-18 &> /dev/null; then
        log_success "ld.lld-18: $(ld.lld-18 --version | head -1)"
    else
        log_error "ld.lld-18 not found"
        failed=1
    fi
    
    # Check LLVM tools
    for tool in llvm-ar-18 llvm-objcopy-18 llvm-objdump-18; do
        if command -v "$tool" &> /dev/null; then
            log_success "$tool: present"
        else
            log_error "$tool not found"
            failed=1
        fi
    done
    
    # Check NASM
    if command -v nasm &> /dev/null; then
        log_success "nasm: $(nasm -version | head -1)"
    else
        log_error "nasm not found"
        failed=1
    fi
    
    # Check Make
    if command -v make &> /dev/null; then
        log_success "make: $(make --version | head -1)"
    else
        log_error "make not found"
        failed=1
    fi
    
    # Check QEMU
    if command -v qemu-system-x86_64 &> /dev/null; then
        log_success "qemu-system-x86_64: $(qemu-system-x86_64 --version | head -1)"
    else
        log_warn "qemu-system-x86_64 not found (optional)"
    fi
    
    # Check GDB
    if command -v gdb &> /dev/null; then
        log_success "gdb: $(gdb --version | head -1)"
    else
        log_warn "gdb not found (optional)"
    fi
    
    # Check Rust
    if command -v rustc &> /dev/null; then
        log_success "rustc: $(rustc --version)"
    else
        log_warn "rustc not found (optional)"
    fi
    
    if [[ $failed -eq 0 ]]; then
        log_success "\n✓ All required toolchain components are installed!"
        return 0
    else
        log_error "\n✗ Some toolchain components are missing!"
        return 1
    fi
}

# Display all pinned versions
show_versions() {
    log_info "LumenOS Toolchain Versions (from toolchain.json)\n"
    
    if [[ ! -f "$TOOLCHAIN_FILE" ]]; then
        log_error "toolchain.json not found at: $TOOLCHAIN_FILE"
        exit 1
    fi
    
    python3 << 'PYTHON'
import json

with open("toolchain.json") as f:
    data = json.load(f)

print(f"Primary Toolchain: {data['primary']['name']}")
print(f"Rationale: {data['primary']['rationale']}\n")

print("Pinned Versions:")
print("-" * 60)

for component, info in data["versions"].items():
    version = info.get("version_full", info.get("version", "N/A"))
    reason = info.get("reason", "")
    print(f"  {component:<15} {version:<20} {reason}")

print("-" * 60)
print(f"\nHost OS: {', '.join(data['host']['os'])}")
print(f"Architectures: {', '.join(data['host']['architectures'])}")
print(f"Recommended: {', '.join(data['host']['recommended_distros'])}")
PYTHON
}

# Main
main() {
    if [[ $# -eq 0 ]]; then
        show_help
        exit 0
    fi
    
    case "$1" in
        --ubuntu)
            if [[ $EUID -ne 0 ]] && ! sudo -n true 2>/dev/null; then
                log_error "This command requires sudo. Please enter your password when prompted."
            fi
            install_ubuntu
            ;;
        --verify)
            verify_toolchain
            ;;
        --versions)
            show_versions
            ;;
        --help|-h)
            show_help
            ;;
        *)
            log_error "Unknown command: $1"
            show_help
            exit 1
            ;;
    esac
}

main "$@"
