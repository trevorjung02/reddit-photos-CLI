const axios = require('axios');
const fs = require('fs');
const { pipeline } = require('stream/promises');
const path = require('path');

main();

async function main() {
    if (process.argv.length < 4) {
        process.exit(1);
    }
    const re_img = /[ab]\.thumbs\.redditmedia\.com\S*\.jpg/g;
    const re_next = /(\?\S*)" rel="nofollow next"/;
    const url = process.argv[2];
    const num_images = process.argv[3];

    await scrape(url, num_images, re_img, re_next)
        .then(async (img_urls) => {
            await download_images(img_urls, path.join(process.cwd(), "images"));
        });
}

async function scrape(url, num_images, re_img, re_next) {
    let img_urls = [];
    let page_num = "";
    while (img_urls.length < num_images) {
        let startLength = img_urls.length;
        let p1 = scrape_urls(url + '.compact' + page_num, re_img, img_urls);
        let p2 = scrape_next_page(url + page_num, re_next);

        let p = await Promise.all([p1, p2])
            .then((res) => {
                if (res[1] == null) {
                    return false;
                }
                if (img_urls.length - startLength < 10) {
                    return false;
                }
                page_num = res[1];
                return true;
            })
        if (!p) {
            console.log(`p = ${p}`);
            break;
        }
    };
    if (img_urls.length > num_images) {
        return img_urls.slice(0, num_images);
    }
    return img_urls;
}

function scrape_urls(url, re_img, img_urls) {
    return axios.get(url)
        .then(function (response) {
            let matches = response.data.match(re_img);
            if (matches != null) {
                img_urls.push(...matches);
            }
        })
        .catch(function (error) {
            console.log(error);
        });
}

function scrape_next_page(url, re_next) {
    return axios.get(url)
        .then(function (response) {
            let matches = response.data.match(re_next);
            if (matches == null) {
                return null;
            }
            return matches[1];
        })
        .catch(function (error) {
            console.log(error);
        });
}

function download_images(img_urls, outPath) {
    let promises = [];
    for (let i = 0; i < img_urls.length; i++) {
        promises.push(downloadFile("https://" + img_urls[i], path.join(outPath, `${i}.jpg`)));
    }
    return Promise.all(promises);
}

function downloadFile(fileUrl, outputLocationPath) {
    return axios({
        method: 'get',
        url: fileUrl,
        responseType: 'stream',
    }).then(async response => {
        return pipeline(response.data, fs.createWriteStream(outputLocationPath));
    });
}