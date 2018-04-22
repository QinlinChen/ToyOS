# Do *NOT* modify this file.

if [ -z $1 ]; then
  goal=build
else
  goal=$1
fi
git add src/ -A --ignore-errors 2> /dev/null
while (test -e .git/index.lock); do 
  sleep 0.1
done
((echo "> ${goal}" && id -un && uname -a) | \
  GIT_COMMITTER_EMAIL="tracer@tracer.org" git commit -F - -q --author='tracer <tracer@tracer.org>' --no-verify --allow-empty) 2> /dev/null \
  || echo -e "\033[1;31mGit commit error. Please report to the teacher/TA if you frequently see this message.\033[0m" > /dev/stderr
sync
