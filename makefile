all:
	make -C bootloader yasab.hex
	cp bootloader/yasab.hex yasab.hex
	make -C bootloader clean
	make -C uploader
	cp uploader/yasab yasab
	make -C uploader clean

clean:
	make -C bootloader clean
	make -C uploader clean
	rm  yasab
	rm  yasab.hex
