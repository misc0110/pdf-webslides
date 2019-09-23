# PDF Web Slides 

This tool allows converting PDF presentation slides to a self-contained HTML5 file. The resulting HTML5 file also contains a presentation mode which mimicks the functionality of [pdfpc](https://github.com/pdfpc/pdfpc). 

TODO: multi-monitor screenshot

# Usage

The first step is to convert a PDF to an HTML5 file. This is simply done by running

    pdf-webslides <pdf file>
    
The output is a standalone `index.html` file in the current directory. If opened, it shows the slides in the same way as the original PDF. Slides can simply be navigated using left/right arrow keys, page-up/page-down keys, as well as by swiping over the slides. 

## Presenter Mode

TODO

## Keyboard Shortcuts

| Key | Description |
|--|--|
| Right / Page Down / Swipe Right to Left |  Go to next slide                                  |
| Left / Page Up / Swipe Left to Right | Go to previous slide                            |
| Home | Go to first slide |
| End | Go to last slide |
| g   | Input slide number to go to |
| +   | Increase note size |
| -   | Decrease note size |
| b   | Turn on/off presentation view, i.e., display an entirely black screen |
| f   | Freeze/unfreeze presentation view. Any change in the presenter mode is not shown on the presentation view |
| r   | Reset timer |
| p   | Display/hide a pointer (i.e., a virtual laser pointer) |

## Notes

The tool supports the same type of notes as pdfpc. Hence, they can also be directly embedded in LaTeX beamer. 
If there are notes in the PDF, they are shown below the preview of the next slide. 

# Compilation

The web-slide converter depends on Poppler and Cairo for converting the PDF to SVGs. 
On Ubuntu, the dependencies can be installed through 

    apt install libcairo2-dev libpoppler-glib-dev
    
Then, the tool can be built by running

    make
    
and 

    make install
    
to install the tool as `pdf-webslides`. 
