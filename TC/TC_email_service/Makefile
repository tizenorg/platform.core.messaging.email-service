testbuildtype=normal
#export testbuildtype 

all:	TCInit TCAccount TCFolder TCRule TCNetwork TCMessage 
	echo done
	
TCInit:
	make -s -C TC_Init	
	
TCAccount:
	make -s -C TC_Account


TCFolder:
	make -s -C TC_Folder 
	
TCRule:
	make -s -C TC_Rule

TCNetwork:
	make -s -C TC_Network 
	
TCMessage:
	make -s -C TC_Message 

clean:
	make clean -C TC_Init
	make clean -C TC_Account
	make clean -C TC_Folder 
	make clean -C TC_Rule
	make clean -C TC_Network 
	make clean -C TC_Message 

