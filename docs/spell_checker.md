# Spell Checking Feature

## Overview

The Immersion application now includes native macOS spell checking for the input text area. This feature uses the macOS NSSpellChecker interface to provide real-time spell checking with suggestions and corrections.

## Features

- **Real-time spell checking**: Words are checked as you type
- **Context menu integration**: Right-click on misspelled words to see suggestions
- **Multiple language support**: Supports all languages available in macOS
- **Word suggestions**: Get spelling suggestions for misspelled words
- **Add to dictionary**: Add custom words to the system dictionary
- **Ignore words**: Temporarily ignore words in the current document
- **Language switching**: Automatically switches based on source language or manually select

## How to Use

### Basic Usage

1. Type text in the input area
2. Misspelled words will be detected automatically
3. Right-click on a misspelled word to see the context menu
4. Choose from spelling suggestions or other options

### Context Menu Options

- **Spelling suggestions**: Click on any suggestion to replace the word
- **Replace...**: Open a dialog to choose from suggestions or enter custom text
- **Add to Dictionary**: Add the word to your personal dictionary
- **Ignore**: Ignore this word for the current document

### Changing Spell Checker Language

1. Go to **Edit** → **Edit spell checker language**
2. Select your desired language from the dropdown
3. The spell checker will immediately switch to the new language

### Automatic Language Detection

The spell checker automatically switches to match your source language:
- English → en_US
- Danish → da_DK
- German → de_DE
- French → fr_FR
- Spanish → es_ES
- And many more...

## Technical Details

### Platform Support

- **macOS**: Full support using native NSSpellChecker
- **Other platforms**: Spell checking is disabled (graceful fallback)

### Dependencies

- AppKit framework (macOS only)
- Foundation framework (macOS only)
- Qt 6 Widgets module

### Implementation

The spell checker is implemented in:
- `include/SpellChecker.h` - Header file
- `src/SpellChecker.cpp` - Implementation
- Integrated into `MainWindow` class

### Status Indicator

The application shows the current spell checker status:
- **Green text**: Spell checker active with language code
- **Red text**: Spell checker not available
- **Gray text**: Spell checker disabled

## Troubleshooting

### Spell Checker Not Working

1. Ensure you're running on macOS
2. Check that the spell checker status shows "Spell checker: [language]" in green
3. Verify that the language you're typing in is supported
4. Try changing the spell checker language manually

### No Suggestions Appearing

1. Make sure the word is actually misspelled
2. Check that the spell checker language matches your input language
3. Try adding the word to your dictionary if it's a valid word

### Language Not Available

1. Install the language in macOS System Preferences
2. Go to System Preferences → Language & Region → Add Language
3. Restart the application after adding the language

## Future Enhancements

- Visual indicators for misspelled words (red underlines)
- Auto-correction suggestions
- Grammar checking integration
- Custom dictionary management
- Cross-platform spell checking support 