import cv2
import numpy as np
import pyautogui
import time

images_dir = "C:/Users/Helfie/Pictures/WowBot" 
images_list = [
"search_button", "buy_button_bottom", "sell_button_bottom",
"close_button", "lowest_price_field", "buy_button"
]

def LocateImage(image, click=None):
    # How to use:
    # search_image("close_button", 1)

    # Load the image and store its dimensions
    image = cv2.imread("{}/{}.png".format(images_dir, image))
    image_width, image_height = image.shape[1], image.shape[0]

    # Get the screenshot of the entire screen
    screenshot = pyautogui.screenshot()

    # Convert the screenshot to a NumPy array
    screenshot = np.array(screenshot)

    # Convert the color channels from BGR to RGB
    screenshot = cv2.cvtColor(screenshot, cv2.COLOR_BGR2RGB)

    # Search for the image in the screenshot
    result = cv2.matchTemplate(screenshot, image, cv2.TM_CCOEFF_NORMED)
    min_val, max_val, min_loc, max_lodc = cv2.minMaxLoc(result)

    # If the match is above a certain threshold, move the cursor to the location of the image
    if max_val > 0.8:
        image_location = max_loc
        cursor_location = (image_location[0] + int(image_width / 2), image_location[1] + int(image_height / 2))
        pyautogui.moveTo(cursor_location)
        if click:
            pyautogui.click()

def MoveToAh():

    # Define the movements as a list of tuples
    movements = [('d', 0.7), ('w', 2.2), ('a', 0.1), ('w', 7.5), 
    ('a', 0.28), ('w', 3), ('a', 0.6), ('w', 2), ('a', 0.1),('w', 0.3)]

    # Loop over the movements and execute them
    for move in movements:
        pyautogui.keyDown(move[0])
        time.sleep(move[1])
        pyautogui.keyUp(move[0])
    TalkToAuctioneer()

def TalkToAuctioneer():
    # Macro on key number 1: /target Auctioneer Fitch
    # Interact with target set to F8
    pyautogui.press('1')
    pyautogui.press('F8')

def ClickMainMonitor():
    screen_width, screen_height = pyautogui.size()

    # Calculate the center point
    center_x = int(screen_width / 2)
    center_y = int(screen_height / 2)

    # Move the cursor to the center point and click
    pyautogui.moveTo(center_x, center_y)
    pyautogui.click()

def main():
    ClickMainMonitor()
    MoveToAh()



if __name__ == '__main__':
    main()
