#!/bin/bash

ip=192.168.1.58
uname=root
image_name=burglar_fucker
builder_name=aarch64_builder
container_name=burglar_fucker_instance
remote=$uname@$ip
root_folder=deploy

function generate_user() {
    read -p "Username:" uname  
    if [ "$uname" == "" ]
    then
        echo "Username cannot be empty"
        exit 2
    fi
    htpasswd -c $root_folder/etc/nginx/.htpasswd $uname
}

function generate_cert() {
    local ssl_dir=$root_folder/etc/ssl
    mkdir -p $ssl_dir/certs $ssl_dir/private
    local openssl_conf=$(mktemp)
    openssl req -x509 -nodes -days 3650 -newkey rsa:4096 -keyout ${ssl_dir}/private/nginx-selfsigned.key -out ${ssl_dir}/certs/nginx-selfsigned.crt -subj "/CN=self.signed"
    openssl dhparam -out ${ssl_dir}/certs/dhparam.pem 4096
}

function start_all() {
    ssh $remote docker run --device /dev/snd --device /dev/video0 --network host --restart=always -d --name $container_name $image_name:latest
    ssh $remote systemctl restart nginx
}

function setup_host_docker() {
    # TODO: check if builder exists, or create it...
    docker buildx use $builder_name
    docker run --rm --privileged linuxkit/binfmt:v0.8
}

function stop_running_container() {
    ssh $remote "docker stop $container_name; docker rm $container_name" &> /dev/null
}

function build_and_upload_image() {
    docker buildx build --platform linux/aarch64 -t $image_name:latest -o type=docker,dest=- . | ssh -C $remote docker load 
}

function upload_overlay() {
    rsync -avr $root_folder/* $remote:/
}

case "$1" in
    deploy)
        setup_host_docker
        stop_running_container
        build_and_upload_image
        ;;

    start)
        start_all
        ;;

    upload)
        upload_overlay
        ;;

    certificate)
        generate_cert
        ;;

    credentials)
        generate_user
        ;;

    *)
        echo "Wrong subcommand"
        exit 1
        ;;
esac

