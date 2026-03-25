.PHONY: default
default:
	@echo "Available Targets:"
	@echo "  - bootstrap  // Compiles required things to build packages"
	@echo "  - initramfs  // Compiles packages for the initramfs if needed and generates"
	@echo "  - run        // Compiles the kernel and runs in qemu w/ KVM"
	@echo "  - tcg        // Compiles the kernel and runs in qemu w/ TCG"
	@echo "  - debug      // Compiles the kernel and runs in qemu w/ TCG & Debugger"
	@echo "  - mkiso      // Makes an ISO that can be ran (Also rebuilds kernel)"

.PHONY: bootstrap
bootstrap:
	./extras/bootstrap.sh
	./extras/generate-initramfs.sh

.PHONY: initramfs
initramfs:
	./extras/generate-initramfs.sh

.PHONY: mkiso
mkiso:
	./extras/compile-kernel.sh
	./extras/generate-iso.sh

.PHONY: run
run:
	./extras/compile-kernel.sh
	./extras/generate-iso.sh
	qemu-system-x86_64 \
		-machine q35,accel=kvm,smm=on -s \
		-cpu host,+x2apic,+invtsc,+pdpe1gb \
		-m 512M \
		-drive if=pflash,format=raw,readonly=on,file=./extras/OVMF_CODE.4m.fd \
		-drive if=pflash,format=raw,readonly=on,file=./extras/OVMF_VARS.4m.fd \
		-cdrom ./evalynOS.iso \
		-boot d -no-reboot -no-shutdown \
		-audiodev pa,id=speaker -machine pcspk-audiodev=speaker \
		-serial stdio

.PHONY: tcg
tcg:
	./extras/compile-kernel.sh
	./extras/generate-iso.sh
	qemu-system-x86_64 \
		-machine q35 \
		-M accel=tcg,smm=on -d int -no-reboot -no-shutdown -D qemu_log.txt \
		-m 512M \
		-drive if=pflash,format=raw,readonly=on,file=./extras/OVMF_CODE.4m.fd \
		-drive if=pflash,format=raw,readonly=on,file=./extras/OVMF_VARS.4m.fd \
		-cdrom ./evalynOS.iso \
		-boot d \
		-audiodev pa,id=speaker -machine pcspk-audiodev=speaker \
		-chardev stdio,id=debugcon \
		-device isa-debugcon,chardev=debugcon

.PHONY: debug
debug:
	./extras/compile-kernel.sh
	./extras/generate-iso.sh
	qemu-system-x86_64 \
		-machine q35 \
		-s -S \
		-M accel=tcg,smm=on -d int -no-reboot -no-shutdown -D qemu_log.txt \
		-m 512M \
		-drive if=pflash,format=raw,readonly=on,file=./extras/OVMF_CODE.4m.fd \
		-drive if=pflash,format=raw,readonly=on,file=./extras/OVMF_VARS.4m.fd \
		-cdrom ./evalynOS.iso \
		-boot d \
		-audiodev pa,id=speaker -machine pcspk-audiodev=speaker \
		-chardev stdio,id=debugcon \
		-device isa-debugcon,chardev=debugcon
