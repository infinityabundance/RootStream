#!/bin/bash

# RootStream Auto-Scaling Script
# This script manages scaling of RootStream deployments

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

NAMESPACE=${NAMESPACE:-rootstream}
DEPLOYMENT_NAME="rootstream"

echo -e "${GREEN}=====================================${NC}"
echo -e "${GREEN}RootStream Auto-Scaling Manager${NC}"
echo -e "${GREEN}=====================================${NC}"
echo ""

# Function to scale deployment
scale_deployment() {
    local replicas=$1
    echo -e "${YELLOW}Scaling $DEPLOYMENT_NAME to $replicas replicas...${NC}"
    kubectl scale deployment/$DEPLOYMENT_NAME -n $NAMESPACE --replicas=$replicas
    echo -e "${GREEN}Scaled successfully!${NC}"
}

# Function to get current scale
get_current_scale() {
    local current=$(kubectl get deployment/$DEPLOYMENT_NAME -n $NAMESPACE -o jsonpath='{.spec.replicas}')
    echo "Current replicas: $current"
    return $current
}

# Function to enable HPA
enable_hpa() {
    local min=$1
    local max=$2
    local cpu_threshold=$3
    
    echo -e "${YELLOW}Enabling HPA...${NC}"
    kubectl autoscale deployment/$DEPLOYMENT_NAME \
        -n $NAMESPACE \
        --min=$min \
        --max=$max \
        --cpu-percent=$cpu_threshold
    
    echo -e "${GREEN}HPA enabled!${NC}"
}

# Function to disable HPA
disable_hpa() {
    echo -e "${YELLOW}Disabling HPA...${NC}"
    kubectl delete hpa/$DEPLOYMENT_NAME -n $NAMESPACE --ignore-not-found=true
    echo -e "${GREEN}HPA disabled!${NC}"
}

# Function to check HPA status
check_hpa() {
    echo -e "${YELLOW}HPA Status:${NC}"
    kubectl get hpa -n $NAMESPACE
}

# Main menu
echo "Select an action:"
echo "1) Manual scale"
echo "2) Enable auto-scaling (HPA)"
echo "3) Disable auto-scaling"
echo "4) Check HPA status"
echo "5) Get current scale"
echo "6) Exit"
echo ""

read -p "Enter choice [1-6]: " choice

case $choice in
    1)
        read -p "Enter number of replicas: " replicas
        scale_deployment $replicas
        ;;
    2)
        read -p "Enter minimum replicas: " min
        read -p "Enter maximum replicas: " max
        read -p "Enter CPU threshold (percentage): " cpu
        enable_hpa $min $max $cpu
        ;;
    3)
        disable_hpa
        ;;
    4)
        check_hpa
        ;;
    5)
        get_current_scale
        ;;
    6)
        echo "Exiting."
        exit 0
        ;;
    *)
        echo -e "${RED}Invalid choice!${NC}"
        exit 1
        ;;
esac

echo ""
echo -e "${GREEN}Operation completed!${NC}"

# Show current pod status
echo ""
echo -e "${YELLOW}Current pod status:${NC}"
kubectl get pods -n $NAMESPACE -l app.kubernetes.io/name=$DEPLOYMENT_NAME
