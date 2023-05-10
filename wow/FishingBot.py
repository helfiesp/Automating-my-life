# Fishingbot for World of Warctaft classic.
# Tested with graphics on Ultra, zoomed in first person and cinematic mode.
# Needs to have a clear view of the water to work properly.

import cv2
import numpy as np
import pyautogui
import time

def main():
    pyautogui.click()
    pyautogui.press('x')
    time.sleep(2)
    # Initialize the background subtractor
    fgbg = cv2.createBackgroundSubtractorMOG2()

    # Get the size of the screen
    screen_size = pyautogui.size()

    # Set minimum contour area threshold
    MIN_CONTOUR_AREA = 100
    iterations = 0


    while True:
        if iterations > 75:
            pyautogui.press('x')
            main()
        time.sleep(0.2)

        # Take a screenshot of the screen
        screenshot = np.array(pyautogui.screenshot())

        # Convert the screenshot to HSV color space
        hsv = cv2.cvtColor(screenshot, cv2.COLOR_RGB2HSV)

        # Define lower and upper bounds for red color
        lower_red = np.array([0, 70, 50])
        upper_red = np.array([10, 255, 255])
        lower_red1 = np.array([170, 70, 50])
        upper_red1 = np.array([180, 255, 255])

        # Threshold the image to get regions containing red color
        mask1 = cv2.inRange(hsv, lower_red, upper_red)
        mask2 = cv2.inRange(hsv, lower_red1, upper_red1)
        mask = cv2.bitwise_or(mask1, mask2)

        # Apply background subtraction to detect movement in the red regions
        fgmask = fgbg.apply(mask)

        # Apply some morphological operations to remove noise and fill in gaps
        kernel = np.ones((5,5),np.uint8)
        fgmask = cv2.erode(fgmask, kernel, iterations = 1)
        fgmask = cv2.dilate(fgmask, kernel, iterations = 2)
        fgmask = cv2.erode(fgmask, kernel, iterations = 1)

        # Find contours in the foreground mask
        contours, hierarchy = cv2.findContours(fgmask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        # If no contours are found, there is no movement
        if not contours:
            iterations += 1
            continue

        # Otherwise, there is movement
        # Get the center of the largest contour
        cnt = max(contours, key=cv2.contourArea)
        M = cv2.moments(cnt)
        cx, cy = int(M['m10']/M['m00']), int(M['m01']/M['m00'])

        # Check if the area of the contour is larger than the threshold
        if cv2.contourArea(cnt) < MIN_CONTOUR_AREA:
            iterations += 1
            continue

        # Scale the cursor position to the screen size
        cx = int(cx * screen_size.width / screenshot.shape[1])
        cy = int(cy * screen_size.height / screenshot.shape[0])

        # Move the mouse cursor to the center of the movement
        if iterations > 20:
            pyautogui.moveTo(cx, cy)
            time.sleep(0.3)
            pyautogui.click(button='right')
            print("Clicked: {}".format(iterations))
            time.sleep(2)
            main()

        # Wait for a short period of time before taking the next screenshot

    cv2.destroyAllWindows()

if __name__ == '__main__':
    main()
