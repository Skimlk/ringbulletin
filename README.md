# ringbulletin
*Status: Work in Progress*

**ringbulletin** is a decentralized content aggregation network built around a federated bulletin board model. It enables users to create and maintain their own boards that reference peers, forming a distributed network of RSS-based content. The system supports blogs, news, announcements, discussions, or any other type of content delivered via RSS.

In addition to federated content discovery, ringbulletin introduces a lightweight reply mechanism: users can respond to other posts by titling their entry in the format `Re: {Original Post Title}`, enabling basic threaded discussions across the network.

## Overview

ringbulletin operates on the principle of federation and distributed content fetching. Each user hosts a simple `board.json` file that defines their own RSS feed and a list of peer boards. The program recursively fetches RSS feeds from peers and their peers up to a defined depth, building a comprehensive and decentralized network of content.

### Key Features

- **Decentralized Networking**: Users control their own boards and define their network by listing peer boards.
- **Recursive Peer Discovery**: Fetches RSS feeds of peers and extends to peers of peers, enhancing content visibility based on search depth.
- **Threaded Replies**: Users can engage in lightweight discussion threads using title-based reply formatting.
- **RSS-Centric Architecture**: Uses standard RSS feeds for content representation and synchronization.
- **Lightweight and Extensible**: Built to be simple yet extensible for various use cases in federated content sharing.

## How It Works

1. Each user hosts a `board.json` file which includes:
   - Their RSS feed URL.
   - A list of peer board URLs.

2. The program parses the user's own board and recursively fetches peer boards and their associated RSS feeds up to a specified level.

3. All fetched posts are aggregated and can be browsed or processed locally or on the web.

4. **Replying to Posts**:  
   Users can reply to any post in the network by creating a new entry in their RSS feed where the title begins with `Re: ` followed by the exact title of the original post.  
   For example:
   - Original post: `My Thoughts on Decentralization`
   - Reply: `Re: My Thoughts on Decentralization`

   This allows ringbulletin to group related posts and represent them as simple threaded discussions.

## Example `board.json`

```json
{
	"title": "Board Title",
	"feeds": [
		"https://feed1.com/index.xml",
		"https://feed2.net/rss"
	],
	"peers": [
		"https://peer1.com/bulletin/board.json",
		"https://peer2.net/board.json"
	]
}
```

## Dependencies

Make sure the following dependencies are installed:

- **cJSON** — for JSON parsing and manipulation
- **libxml2** — for parsing RSS/XML feeds and generating HTML pages
- **xxhash** — for hashing post titles, used in the reply/threading feature
- **libcurl** — for HTTP requests and fetching remote content

## Installation

Clone the repository and build:

```bash
git clone https://github.com/Skimlk/ringbulletin
cd ringbulletin
make
```
