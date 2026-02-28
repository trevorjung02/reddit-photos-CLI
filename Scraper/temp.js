const axios = require('axios');
const fs = require('fs');
const { pipeline } = require('stream/promises');
const path = require('path');

main();

async function main() {
    // if (process.argv.length < 4) {
    //     process.exit(1);
    // }
    // const url = process.argv[2];
    // const num_images = process.argv[3];
    // const outDir = process.argv[4];
    const url = 'https://old.reddit.com/r/naturephotography/';
    const num_images = 50;

    const imgUrls = await getImgUrlsFromSub(url, num_images);
    console.log(imgUrls);
}

function getImgUrls(data) {
    const re = /(?<=src="\/\/)preview.redd.it\S*(?=")/g;
    return data.match(re)
        .map(s => s.replaceAll('&amp;', '&'));
}

function getNextPageQuery(data) {
    const re = /\?count=\S*(?=")/g;
    return data.match(re).at(-1).replaceAll('&amp;', '&');
}

async function getImgUrlsFromSub(url, num_images) {
    let img_urls = [];
    let pageQuery = "";
    while (img_urls.length < num_images) {
        const response = await axios.get(url + pageQuery)
        img_urls.push(...getImgUrls(response.data));
        pageQuery = getNextPageQuery(response.data);
    }
    if (img_urls.length > num_images) {
        return img_urls.slice(0, num_images);
    }
    return img_urls;
}
