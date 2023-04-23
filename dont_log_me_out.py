# I was playing WoW Classic and kept getting stuck in queues to get on the game
# I created this script to not log me out of the game

import random
import time
from PIL import ImageGrab, Image
import pyautogui
import win32gui
import pyperclip
import os


launcher_file = "C:/Program Files (x86)/World of Warcraft/World of Warcraft Launcher.exe"

def main():
	pyautogui.moveTo(900, 430)
	pyautogui.click();
	if LogoCheck():
		print("[*] Logo is visible...")
		if CharScreenCheck():
			print("[*] At the character selection screen...")
			pyautogui.press('enter')
		if LogScreenCheck():
			print("[*] At the login screen...")
			if ReconnectCheck():
				pyautogui.press('enter')
			if LoginFieldCheck():
				print("[-] Login fields are active...")
				exit()
	else:
		print("[*] Character is likely ingame...")
	time.sleep(30)

def CharScreenCheck():
	# Checks the value of the shop button, aswell as the realm button
	# Different values are accepted depending on if they are highlighted
	shop_button = ImageGrab.grab(bbox =(80, 955, 105, 980)).getpixel((10,10))[0]
	realm_button = ImageGrab.grab(bbox =(1650, 67, 1670, 83)).getpixel((10,10))[0]
	shop_button_values = [115, 210]
	realm_button_values = [105, 173, 255]
	if shop_button in shop_button_values and realm_button in realm_button_values:
		return True

def LogScreenCheck():
	# Checks the value of the blue in the quit button on the login screen
	quit_button = ImageGrab.grab(bbox =(1850, 1000, 1885, 1020)).getpixel((10,10))
	quit_button_values = [20,21,22,51]
	if quit_button[0] in quit_button_values:
		return True

def LogoCheck():
	# Checks if the World of Warcraft logo is present in the top left corner.
	# Default values: 35, 105, 168
	logo_check = ImageGrab.grab(bbox =(130, 60, 250, 120)).getpixel((10,10))
	logo_check_values = [35]
	if logo_check[0] in logo_check_values:
		return True

def ReconnectCheck():
	# Checks if the reconnect button is visible or not
	reconnect_button = ImageGrab.grab(bbox =(920, 610, 1000, 640)).getpixel((10,10))
	reconnect_button_values = [25, 51]
	if reconnect_button[0] in reconnect_button_values:
		return True

def LoginFieldCheck():
	# Checks if the email/password fields are visible
	email_button = ImageGrab.grab(bbox =(960, 550, 990, 580)).getpixel((10,10))[0]
	password_button = ImageGrab.grab(bbox =(920, 665, 950, 685)).getpixel((10,10))[0]
	if email_button < 25 and password_button < 10:
		return True

def StartGame():
	# Starts the BNET Launcher and clicks the play button
	os.startfile(launcher_file)
	time.sleep(20)
	pyautogui.moveTo(175, 950)
	pyautogui.click()
	print("[-] Launching game...")
	time.sleep(20)

if __name__ == '__main__':
	print("----- Starting ANTI-AFK script -----")
	StartGame()
	while True:
		main()
