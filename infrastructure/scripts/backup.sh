#!/bin/bash

# RootStream Backup Script
# This script handles backup of RootStream data and configuration

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

NAMESPACE=${NAMESPACE:-rootstream}
BACKUP_DIR=${BACKUP_DIR:-/tmp/rootstream-backups}
TIMESTAMP=$(date +%Y%m%d-%H%M%S)
S3_BUCKET=${S3_BUCKET:-rootstream-backups}

echo -e "${GREEN}=====================================${NC}"
echo -e "${GREEN}RootStream Backup Manager${NC}"
echo -e "${GREEN}=====================================${NC}"
echo ""

# Create backup directory
mkdir -p $BACKUP_DIR/$TIMESTAMP

# Function to backup database
backup_database() {
    echo -e "${YELLOW}Backing up database...${NC}"
    
    # Get database credentials from secrets
    DB_HOST=$(kubectl get secret rootstream-db-secret -n $NAMESPACE -o jsonpath='{.data.host}' | base64 -d)
    DB_NAME=$(kubectl get secret rootstream-db-secret -n $NAMESPACE -o jsonpath='{.data.database}' | base64 -d)
    DB_USER=$(kubectl get secret rootstream-db-secret -n $NAMESPACE -o jsonpath='{.data.username}' | base64 -d)
    DB_PASS=$(kubectl get secret rootstream-db-secret -n $NAMESPACE -o jsonpath='{.data.password}' | base64 -d)
    
    # Create database dump
    PGPASSWORD=$DB_PASS pg_dump -h $DB_HOST -U $DB_USER -d $DB_NAME \
        > $BACKUP_DIR/$TIMESTAMP/database-backup.sql
    
    echo -e "${GREEN}Database backup completed!${NC}"
}

# Function to backup Kubernetes resources
backup_k8s_resources() {
    echo -e "${YELLOW}Backing up Kubernetes resources...${NC}"
    
    # Backup deployments
    kubectl get deployments -n $NAMESPACE -o yaml > $BACKUP_DIR/$TIMESTAMP/deployments.yaml
    
    # Backup services
    kubectl get services -n $NAMESPACE -o yaml > $BACKUP_DIR/$TIMESTAMP/services.yaml
    
    # Backup configmaps
    kubectl get configmaps -n $NAMESPACE -o yaml > $BACKUP_DIR/$TIMESTAMP/configmaps.yaml
    
    # Backup secrets (be careful with this in production!)
    kubectl get secrets -n $NAMESPACE -o yaml > $BACKUP_DIR/$TIMESTAMP/secrets.yaml
    
    # Backup ingress
    kubectl get ingress -n $NAMESPACE -o yaml > $BACKUP_DIR/$TIMESTAMP/ingress.yaml
    
    # Backup PVCs
    kubectl get pvc -n $NAMESPACE -o yaml > $BACKUP_DIR/$TIMESTAMP/pvcs.yaml
    
    echo -e "${GREEN}Kubernetes resources backup completed!${NC}"
}

# Function to backup persistent volumes
backup_persistent_data() {
    echo -e "${YELLOW}Backing up persistent volume data...${NC}"
    
    # Get list of PVCs
    pvcs=$(kubectl get pvc -n $NAMESPACE -o jsonpath='{.items[*].metadata.name}')
    
    for pvc in $pvcs; do
        echo "Backing up PVC: $pvc"
        
        # Create a temporary pod to access the PVC
        kubectl run backup-pod-$RANDOM \
            --image=busybox \
            --restart=Never \
            -n $NAMESPACE \
            --overrides='
            {
              "spec": {
                "containers": [{
                  "name": "backup",
                  "image": "busybox",
                  "command": ["tar", "czf", "/backup/data.tar.gz", "/data"],
                  "volumeMounts": [{
                    "name": "data",
                    "mountPath": "/data"
                  }, {
                    "name": "backup",
                    "mountPath": "/backup"
                  }]
                }],
                "volumes": [{
                  "name": "data",
                  "persistentVolumeClaim": {
                    "claimName": "'$pvc'"
                  }
                }, {
                  "name": "backup",
                  "hostPath": {
                    "path": "'$BACKUP_DIR/$TIMESTAMP'"
                  }
                }]
              }
            }'
        
        # Wait for completion and cleanup
        kubectl wait --for=condition=complete pod/backup-pod-$RANDOM -n $NAMESPACE --timeout=300s
        kubectl delete pod backup-pod-$RANDOM -n $NAMESPACE
    done
    
    echo -e "${GREEN}Persistent data backup completed!${NC}"
}

# Function to compress backup
compress_backup() {
    echo -e "${YELLOW}Compressing backup...${NC}"
    
    cd $BACKUP_DIR
    tar -czf rootstream-backup-$TIMESTAMP.tar.gz $TIMESTAMP/
    rm -rf $TIMESTAMP/
    
    echo -e "${GREEN}Backup compressed: rootstream-backup-$TIMESTAMP.tar.gz${NC}"
}

# Function to upload to S3
upload_to_s3() {
    echo -e "${YELLOW}Uploading backup to S3...${NC}"
    
    aws s3 cp $BACKUP_DIR/rootstream-backup-$TIMESTAMP.tar.gz \
        s3://$S3_BUCKET/backups/ \
        --storage-class STANDARD_IA
    
    echo -e "${GREEN}Backup uploaded to S3!${NC}"
}

# Function to cleanup old backups
cleanup_old_backups() {
    local retention_days=${1:-7}
    
    echo -e "${YELLOW}Cleaning up backups older than $retention_days days...${NC}"
    
    # Cleanup local backups
    find $BACKUP_DIR -name "rootstream-backup-*.tar.gz" -mtime +$retention_days -delete
    
    # Cleanup S3 backups (using lifecycle policy would be better)
    echo "Note: Set up S3 lifecycle policy for automated cleanup"
    
    echo -e "${GREEN}Cleanup completed!${NC}"
}

# Main backup process
echo "Starting backup process..."
echo ""

backup_database
backup_k8s_resources
backup_persistent_data
compress_backup

read -p "Upload backup to S3? (yes/no): " upload_s3
if [ "$upload_s3" == "yes" ]; then
    upload_to_s3
fi

read -p "Cleanup old backups? (yes/no): " cleanup
if [ "$cleanup" == "yes" ]; then
    read -p "Enter retention days (default: 7): " retention
    retention=${retention:-7}
    cleanup_old_backups $retention
fi

echo ""
echo -e "${GREEN}=====================================${NC}"
echo -e "${GREEN}Backup completed successfully!${NC}"
echo -e "${GREEN}=====================================${NC}"
echo ""
echo "Backup location: $BACKUP_DIR/rootstream-backup-$TIMESTAMP.tar.gz"
echo ""
echo "To restore from this backup:"
echo "  tar -xzf rootstream-backup-$TIMESTAMP.tar.gz"
echo "  kubectl apply -f $TIMESTAMP/"
echo ""
