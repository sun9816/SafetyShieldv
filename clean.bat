@echo off
del /q SafetyShield.sdf
del /q SafetyShield.VC.db
rd /q /s Debug
rd /q /s Release
rd /q /s PortMap\Debug
rd /q /s PortMap\Release
rd /q /s Common\Debug
rd /q /s Common\Release
rd /q /s TextDataEncrypt\Debug
rd /q /s TextDataEncrypt\Release