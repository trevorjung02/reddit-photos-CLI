# reddit-photos-CLI
This program creates photomosaics out of images scraped from Reddit. An example photomosaic side by side with the original is shown below:
<p float="left">
<img src="https://res.cloudinary.com/emrys/image/upload/v1641013582/photomosaicsGit/puyhv4pxsw881_rpqtut.jpg" alt="photomosaic" width="400"/>
<img src="https://res.cloudinary.com/emrys/image/upload/v1641013637/photomosaicsGit/puyhv4pxsw881_1_cseynz.jpg" alt="original" width="400"/>
</p>

  
## Instructions for Windows users:
Download and unzip the latest release. No need to install.
### How To Run
Run the file redditphotos.exe in the command line. The required argument is a path to the input image. Example shown below scrapes from r/pics: \
`redditphotos.exe" --sub=pics C:\path\to\image.jpg `

Additional options:
| Flag | Use |
| ---- | --- |
| --sub | Subreddit to scrape images from. Default is EarthPorn (This is SFW, just inconveniently named. It is the largest nature pic subreddit.) |
| --n  | Number of images to scrape. Default is 100. |
| --tile_size | Side length of each "tile" image. Default is 25. |
| --scale | Scale factor to apply to image, Default is 1. Note: the photomosaics will be very blocky if the scale factor is too small. |
| --o | Name of output image. Default is same as input image. |

Example using all flags: \
`redditphotos.exe --sub=pics --n=100 --tile_size=25 --scale=5 --o="myimage.png" C:\path\to\image.jpg`

## Cloning/Dependencies
If you want to clone this project and build it yourself, note that you will need the OpenCV library. 
