##############################################################################
# Multi-project makefile rules
#

all:
	@echo
	@echo === Building for STM32G474RE-Nucleo64 ==============================
	+@make --no-print-directory -f ./make/stm32g474re_nucleo64.make all
	@echo ====================================================================
	@echo
	@echo === Building for STM32H563ZI-Nucleo144 =============================
	+@make --no-print-directory -f ./make/stm32h563zi_nucleo144.make all
	@echo ====================================================================
	@echo
	@echo
	@echo === Building for STM32H743ZI-Nucleo144 =============================
	+@make --no-print-directory -f ./make/stm32h743zi_nucleo144.make all
	@echo ====================================================================
	@echo
	@echo === Building for STM32L476-Discovery ===============================
	+@make --no-print-directory -f ./make/stm32l476_discovery.make all
	@echo ====================================================================
	@echo
	@echo === Building for STM32L4R5ZI-Nucleo144 =============================
	+@make --no-print-directory -f ./make/stm32l4r5zi_nucleo144.make all
	@echo ====================================================================
	@echo
	@echo === Building for STM32WL55JC_Nucleo64 ==============================
	+@make --no-print-directory -f ./make/stm32wl55jc_nucleo64.make all
	@echo ====================================================================
	@echo

clean:
	@echo
	+@make --no-print-directory -f ./make/stm32g474re_nucleo64.make clean
	@echo
	+@make --no-print-directory -f ./make/stm32h563zi_nucleo144.make clean
	@echo
	+@make --no-print-directory -f ./make/stm32h743zi_nucleo144.make clean
	@echo
	+@make --no-print-directory -f ./make/stm32l476_discovery.make clean
	@echo
	+@make --no-print-directory -f ./make/stm32l4r5zi_nucleo144.make clean
	@echo
	+@make --no-print-directory -f ./make/stm32wl55jc_nucleo64.make clean
	@echo

#
##############################################################################
