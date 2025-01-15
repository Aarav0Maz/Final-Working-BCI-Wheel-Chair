# Import necessary libraries
import numpy as np  # For numerical operations, including array manipulation
import pyautogui  # For automating GUI tasks such as pressing keys
import time  # For time-related operations, e.g., timestamping and delays
from pylsl import StreamInlet, resolve_byprop  # For connecting to and reading data from LSL streams (EEG data in this case)
import utils  # Assuming 'utils' contains utility functions for EEG signal processing

# Define the EEG frequency bands as a class
class Band:
    Delta = 0  # Delta band (low frequency)
    Theta = 1  # Theta band
    Alpha = 2  # Alpha band
    Beta = 3   # Beta band

# Signal processing parameters
BUFFER_LENGTH = 5  # Length of the buffer to store EEG data in seconds
EPOCH_LENGTH = 1  # Length of each epoch for processing in seconds
OVERLAP_LENGTH = 0.8  # Overlap between consecutive epochs
SHIFT_LENGTH = EPOCH_LENGTH - OVERLAP_LENGTH  # Shift length between consecutive epochs
INDEX_CHANNEL = 3  # The index of the EEG channel we are interested in

# Blink detection parameters
BLINK_THRESHOLD = 1.0  # Threshold value to detect a blink in the signal
BLINK_TIMEOUT = 1.0  # Time window (in seconds) to detect the second blink (after the first one)

# BlinkDetector class for detecting and processing blinks
class BlinkDetector:
    def __init__(self):
        self.last_blink_time = 0  # Initialize the last blink time
        self.last_blink_value = None  # Track the last blink value (either 1 or 0)
        self.is_waiting_for_blink = False  # Flag to check if we're waiting for the next blink

    def process_blink(self, delta_value, prev_delta):
        current_time = time.time()  # Get the current time for timestamping the blink

        # Check if the delta value crosses the blink threshold (blink detected)
        if delta_value >= BLINK_THRESHOLD and prev_delta <= BLINK_THRESHOLD:
            if not self.is_waiting_for_blink:
                # If we're not currently waiting for a blink, it means we detected a blink
                if self.last_blink_value is None:
                    # If it's the first blink ever detected, set it to '1'
                    print("First blink detected! Pressing 1 followed by Enter")
                    pyautogui.press('1')  # Simulate pressing '1'
                    pyautogui.press('enter')  # Simulate pressing 'Enter'
                    self.last_blink_value = 1  # Update the last blink value to 1
                else:
                    # If the blink value is already set, toggle between 1 and 0
                    next_blink_value = 0 if self.last_blink_value == 1 else 1
                    print(f"Blink detected! Pressing {next_blink_value} followed by Enter")
                    pyautogui.press(str(next_blink_value))  # Simulate pressing the toggled value (0 or 1)
                    pyautogui.press('enter')  # Simulate pressing 'Enter'
                    self.last_blink_value = next_blink_value  # Update the last blink value to the new toggled value

                self.is_waiting_for_blink = True  # Set flag to wait for the next blink
                self.last_blink_time = current_time  # Update the last blink time to the current time

        # If the timeout period has passed without a blink, reset the waiting state
        if (current_time - self.last_blink_time) > BLINK_TIMEOUT:
            self.is_waiting_for_blink = False

# Main execution starts here
if __name__ == "__main__":
    print('Looking for an EEG stream...')  # Log message indicating stream lookup
    streams = resolve_byprop('type', 'EEG', timeout=2)  # Resolve EEG stream (timeout 2 seconds)
    if len(streams) == 0:  # If no EEG stream found, raise an error
        raise RuntimeError('Can\'t find EEG stream.')

    print("Start acquiring data")  # Log message indicating data acquisition has started
    inlet = StreamInlet(streams[0], max_chunklen=12)  # Initialize stream inlet to receive EEG data
    fs = int(inlet.info().nominal_srate())  # Get the sampling rate (frequency) of the EEG stream

    # Initialize buffers for processing EEG data
    eeg_buffer = np.zeros((int(fs * BUFFER_LENGTH), 1))  # Create a buffer for EEG data (length in samples)
    filter_state = None  # Placeholder for filter state (for signal filtering)
    n_win_test = int(np.floor((BUFFER_LENGTH - EPOCH_LENGTH) / SHIFT_LENGTH + 1))  # Number of windows for testing
    band_buffer = np.zeros((n_win_test, 4))  # Buffer for band powers (4 frequency bands)

    # Initialize the BlinkDetector
    blink_detector = BlinkDetector()

    print('Press Ctrl-C in the console to break the while loop.')  # Log message to inform user how to exit
    allValues = [0]  # Initialize a list to track all delta wave values

    try:
        while True:  # Start a continuous loop for data acquisition
            # Pull EEG data chunk from the stream
            eeg_data, timestamp = inlet.pull_chunk(timeout=1, max_samples=int(SHIFT_LENGTH * fs))

            ch_data = np.array(eeg_data)[:, INDEX_CHANNEL]  # Extract data from the desired EEG channel
            # Update the buffer with the new EEG data and apply any necessary filters
            eeg_buffer, filter_state = utils.update_buffer(eeg_buffer, ch_data, notch=True, filter_state=filter_state)

            # Extract the most recent data epoch for further analysis
            data_epoch = utils.get_last_data(eeg_buffer, EPOCH_LENGTH * fs)
            band_powers = utils.compute_band_powers(data_epoch, fs)  # Compute the band powers from the data epoch
            band_buffer, _ = utils.update_buffer(band_buffer, np.asarray([band_powers]))  # Update the band power buffer

            deltaWaves = band_powers[Band.Delta]  # Extract delta band power (focus on delta waves)
            allValues.append(deltaWaves)  # Add the delta value to the list
            print("Delta: {}".format(deltaWaves))  # Print the current delta wave power value

            # Process the blink detection logic with the current and previous delta values
            blink_detector.process_blink(deltaWaves, allValues[-2])  # Compare the current delta with the previous one

    except KeyboardInterrupt:  # Handle manual interruption (Ctrl-C)
        print('Closing!')  # Log message to indicate the script is closing

