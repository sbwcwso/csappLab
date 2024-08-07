#!/bin/bash
#!/bin/bash

curl_request() {
  random_number=$(printf "%02d" $((RANDOM % 17)))
  url="http://localhost:123/Shakespeare_${random_number}.txt"
  curl -s -o /dev/null --proxy http://localhost:12345 $url
}

export -f curl_request

seq 1 400 | parallel -j 4 curl_request

