
.text
.set noreorder

.global _tlbRefillHandler
.ent _tlbRefillHandler
# I'm sure setting the handler again isn't necessary, but here's what this does
# If the handler is called, set the tlb entry to the desired virtual address
# But if the emulator doesn't support tlb remapping, this would still do nothing
# We skip the bad instruction anyways, this allows us to still read the flag
_tlbRefillHandler:
	mfc0 $k0, $8    # Load the bad address
	mtc0 $zero, $0  # Set our index to 0
	tlbr            # Read our entry
	sync.p
	mtc0 $k0, $10   # Set our EntryHi to the bad virtual address
	tlbwi           # Write back to our entry
	sync.p

	# Set the flag
	li $k0, 1
	sb $k0, _tlbRefillHandlerCalled

	mfc0 $k0, $30    # Get the PC that caused the exception
	sync.p
	addi $k0, $k0, 4 # Skip an instruction
	mtc0 $k0, $30    # Set the new ErrorPC
	sync.p
	eret             # Exit this handler
	nop
.end _tlbRefillHandler
